
#include <utility>
#include <nlohmann/json.hpp>

#include "Folder.hpp"
#include "Backend.hpp"
#include "File.hpp"

/*****************************************************/
Folder::Folder(Backend& backend) : 
    Item(backend), debug("Folder",this)
{
    debug << __func__ << "()"; debug.Info();    
}

/*****************************************************/
Folder::Folder(Backend& backend, const nlohmann::json& data, bool haveItems) : 
    Item(backend, data), debug("Folder",this)
{
    debug << __func__ << "()"; debug.Info();
    
    try
    {
        data.at("counters").at("size").get_to(this->size);
        // TODO filesystems/etc. that extend this will not have size...
        // need to abstract GetSize() from data (static?)

        if (haveItems)
        {
            this->LoadItems(data);
            this->haveItems = true;
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}

/*****************************************************/
Folder::Folder(Backend& backend, Folder& parent, const nlohmann::json& data, bool haveItems) : 
    Folder(backend, data, haveItems)
{
    this->parent = &parent;
}

/*****************************************************/
Item& Folder::GetItemByPath(std::string path)
{
    this->debug << __func__ << "(path:" << path << ")"; this->debug.Info();

    if (path[0] == '/') path.erase(0,1);

    if (path.empty()) return *this;

    const auto [name, subpath] = Utilities::split(path,"/");

    this->debug << __func__ << "... name:" << name 
        << " subpath:" << subpath; this->debug.Details();

    const ItemMap& items = GetItems();
    ItemMap::const_iterator it = items.find(name);
    if (it == items.end()) throw Backend::NotFoundException();

    Item& item = *(it->second);

    // TODO replace with an iterative verison? (see server)

    if (subpath.empty()) return item;
    else if (item.GetType() != Item::Type::FOLDER) 
        throw NotFolderException();
    else 
    {
        Folder& folder = dynamic_cast<Folder&>(item);
        return folder.GetItemByPath(subpath);
    }
}

/*****************************************************/
File& Folder::GetFileByPath(const std::string& path)
{
    Item& item = GetItemByPath(path);

    if (item.GetType() != Item::Type::FILE)
        throw NotFileException();

    return dynamic_cast<File&>(item);
}

/*****************************************************/
Folder& Folder::GetFolderByPath(const std::string& path)
{
    Item& item = GetItemByPath(path);

    if (item.GetType() != Item::Type::FOLDER)
        throw NotFolderException();

    return dynamic_cast<Folder&>(item);
}

/*****************************************************/
const Folder::ItemMap& Folder::GetItems()
{
    this->debug << __func__ << "()"; this->debug.Info();

    if (!this->haveItems) LoadItems();

    this->haveItems = true;
    return this->itemMap;
}

/*****************************************************/
void Folder::LoadItems()
{
    debug << __func__ << "()"; debug.Info();

    this->LoadItems(backend.GetFolder(this->id));
}

/*****************************************************/
void Folder::LoadItems(const nlohmann::json& data)
{
    debug << __func__ << "()"; debug.Info();

    this->itemMap.clear();

    try
    {
        for (const nlohmann::json& el : data.at("files"))
        {
            std::unique_ptr<File> file(std::make_unique<File>(backend, *this, el));

            this->itemMap[file->GetName()] = std::move(file);
        }

        for (const nlohmann::json& el : data.at("folders"))
        {
            std::unique_ptr<Folder> folder(std::make_unique<Folder>(backend, *this, el));

            this->itemMap[folder->GetName()] = std::move(folder);
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}

/*****************************************************/
void Folder::CreateFile(const std::string& name)
{
    const ItemMap& items = GetItems(); // pre-populate items

    if (items.count(name)) throw DuplicateItemException();

    throw Utilities::Exception("not implemented"); // TODO implement me
}

/*****************************************************/
void Folder::CreateFolder(const std::string& name)
{
    const ItemMap& items = GetItems(); // pre-populate items

    if (items.count(name)) throw DuplicateItemException();

    nlohmann::json data(backend.CreateFolder(this->id, name));

    std::unique_ptr<Folder> folder(std::make_unique<Folder>(backend, *this, data));

    this->itemMap[folder->GetName()] = std::move(folder);
}

/*****************************************************/
void Folder::RemoveItem(const std::string& name)
{
    const ItemMap& items = GetItems();

    ItemMap::const_iterator it = items.find(name);

    if (it != items.end()) itemMap.erase(name);
}

/*****************************************************/
void Folder::Delete()
{
    backend.DeleteFolder(this->id);

    if (this->parent != nullptr)
        this->parent->RemoveItem(this->name);
}
