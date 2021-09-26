#include <utility>
#include <nlohmann/json.hpp>

#include "Folder.hpp"
#include "File.hpp"
#include "Backend.hpp"
#include "folders/PlainFolder.hpp"

/*****************************************************/
Folder::Folder(Backend& backend) : 
    Item(backend), debug("Folder",this)
{
    debug << __func__ << "()"; debug.Info();
}

/*****************************************************/
Folder::Folder(Backend& backend, const nlohmann::json& data) : 
    Item(backend, data), debug("Folder",this)
{
    debug << __func__ << "()"; debug.Info();
}

/*****************************************************/
Folder& Folder::GetParent() const     
{ 
    if (this->parent == nullptr) 
        throw NullParentException();
            
    return *this->parent; 
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
    this->debug << __func__ << "(name:" << this->name << ")"; this->debug.Info();

    if (!this->haveItems) LoadItems();

    this->haveItems = true;
    return this->itemMap;
}

/*****************************************************/
void Folder::LoadItemsFrom(const nlohmann::json& data)
{
    debug << __func__ << "()"; debug.Info();

    try
    {
        for (const nlohmann::json& el : data.at("files"))
        {
            std::unique_ptr<File> file(std::make_unique<File>(backend, *this, el));

            debug << __func__ << "... file:" << file->GetName(); debug.Details();

            this->itemMap[file->GetName()] = std::move(file);
        }

        for (const nlohmann::json& el : data.at("folders"))
        {
            std::unique_ptr<PlainFolder> folder(std::make_unique<PlainFolder>(backend, *this, el));

            debug << __func__ << "... folder:" << folder->GetName(); debug.Details();

            this->itemMap[folder->GetName()] = std::move(folder);
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }

    this->haveItems = true;
}

/*****************************************************/
void Folder::RemoveItem(const std::string& name)
{
     debug << __func__ << "(name:" << name << ")"; debug.Info();

    const ItemMap& items = GetItems();

    ItemMap::const_iterator it = items.find(name);

    if (it != items.end())
    {
        SubRemoveItem(*(it->second));

        itemMap.erase(it);
    }
}

/*****************************************************/
void Folder::CreateFile(const std::string& name)
{
    debug << __func__ << "(name:" << name << ")"; debug.Info();

    const ItemMap& items = GetItems(); // pre-populate items

    if (items.count(name)) throw DuplicateItemException();

    SubCreateFile(name);
}

/*****************************************************/
void Folder::CreateFolder(const std::string& name)
{
    debug << __func__ << "(name:" << name << ")"; debug.Info();

    const ItemMap& items = GetItems(); // pre-populate items

    if (items.count(name)) throw DuplicateItemException();

    SubCreateFolder(name);
}
