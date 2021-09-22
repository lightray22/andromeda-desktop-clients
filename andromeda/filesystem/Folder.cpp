
#include <utility>
#include <nlohmann/json.hpp>

#include "Folder.hpp"
#include "Backend.hpp"
#include "File.hpp"

/*****************************************************/
std::unique_ptr<Folder> Folder::LoadByID(Backend& backend, const std::string& id)
{
    backend.RequireAuthentication();

    nlohmann::json data(backend.GetFolder(id));

    return std::make_unique<Folder>(backend, data, true);
}

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
Item& Folder::GetItemByPath(const std::string& path)
{
    this->debug << __func__ << "(path:" << path << ")"; this->debug.Info();

    if (path.empty()) return *this;

    const auto [name, subpath] = Utilities::split(path,"/");

    this->debug << __func__ << "... name:" << name << " subpath:" << subpath; this->debug.Details();

    ItemMap::iterator it = itemMap.find(name);
    if (it == itemMap.end()) throw Backend::NotFoundException();

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
            std::unique_ptr<File> file(std::make_unique<File>(backend, el));

            this->itemMap[file->GetName()] = std::move(file);
        }

        for (const nlohmann::json& el : data.at("folders"))
        {
            std::unique_ptr<Folder> folder(std::make_unique<Folder>(backend, el, false));

            this->itemMap[folder->GetName()] = std::move(folder);
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}

