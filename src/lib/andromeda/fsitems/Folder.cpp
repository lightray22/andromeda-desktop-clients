#include <utility>
#include <nlohmann/json.hpp>

#include "Folder.hpp"
#include "File.hpp"
#include "andromeda/Backend.hpp"
#include "andromeda/fsitems/folders/PlainFolder.hpp"

using namespace std::chrono;

namespace Andromeda {
namespace FSItems {

/*****************************************************/
Folder::Folder(Backend& backend) : 
    Item(backend), debug("Folder",this)
{
    debug << __func__ << "()"; debug.Info();
}

/*****************************************************/
Item& Folder::GetItemByPath(std::string path)
{
    this->debug << this->name << ":" << __func__ << "(path:" << path << ")"; this->debug.Info();

    if (path[0] == '/') path.erase(0,1);

    if (path.empty()) return *this;    

    Utilities::StringList parts = Utilities::explode(path,"/");

    // iteratively (not recursively) find the correct parent/subitem
    Folder* parent_ = this; 
    for (Utilities::StringList::iterator pIt = parts.begin(); 
        pIt != parts.end(); ++pIt )
    {
        const ItemMap& items = parent_->GetItems();
        ItemMap::const_iterator it = items.find(*pIt);
        if (it == items.end()) throw NotFoundException();

        Item& item = *(it->second);

        if (std::next(pIt) == parts.end()) return item; // last part of path

        if (item.GetType() != Type::FOLDER) throw NotFolderException();

        parent_ = dynamic_cast<Folder*>(&item);
    }

    throw NotFoundException(); // should never get here
}

/*****************************************************/
File& Folder::GetFileByPath(const std::string& path)
{
    Item& item = GetItemByPath(path);

    if (item.GetType() != Type::FILE)
        throw NotFileException();

    return dynamic_cast<File&>(item);
}

/*****************************************************/
Folder& Folder::GetFolderByPath(const std::string& path)
{
    Item& item = GetItemByPath(path);

    if (item.GetType() != Type::FOLDER)
        throw NotFolderException();

    return dynamic_cast<Folder&>(item);
}

/*****************************************************/
const Folder::ItemMap& Folder::GetItems()
{
    bool expired = (steady_clock::now() - this->refreshed)
        > backend.GetConfig().GetOptions().refreshTime;

    bool noCache = backend.GetConfig().GetOptions().cacheType == Config::Options::CacheType::NONE; // load always
    bool memory  = backend.GetConfig().GetOptions().cacheType == Config::Options::CacheType::MEMORY; // load once

    if (!this->haveItems || (expired && !memory) || noCache) 
    {
        LoadItems(); this->refreshed = steady_clock::now();
    }

    this->haveItems = true;
    return this->itemMap;
}

/*****************************************************/
void Folder::LoadItemsFrom(const nlohmann::json& data)
{
    debug << this->name << ":" << __func__ << "()"; debug.Info();

    Folder::NewItemMap newItems;

    NewItemFunc newFile = [&](const nlohmann::json& fileJ)->std::unique_ptr<Item> {
        return std::make_unique<File>(backend, fileJ, *this); };

    NewItemFunc newFolder = [&](const nlohmann::json& folderJ)->std::unique_ptr<Item> {
        return std::make_unique<Folders::PlainFolder>(backend, &folderJ, this); };

    try
    {
        for (const nlohmann::json& fileJ : data.at("files"))
            newItems.emplace(std::piecewise_construct,
                std::forward_as_tuple(fileJ.at("name")), std::forward_as_tuple(fileJ, newFile));

        for (const nlohmann::json& folderJ : data.at("folders"))
            newItems.emplace(std::piecewise_construct,
                std::forward_as_tuple(folderJ.at("name")), std::forward_as_tuple(folderJ, newFolder));
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }

    SyncContents(newItems);

    this->haveItems = true;
    this->refreshed = steady_clock::now();
}

/*****************************************************/
void Folder::SyncContents(const Folder::NewItemMap& newItems)
{
    debug << this->name << ":" << __func__ << "()"; debug.Info();

    for (const NewItemMap::value_type& newIt : newItems)
    {
        const std::string& name_(newIt.first);
        const nlohmann::json& data(newIt.second.first);

        ItemMap::const_iterator existIt(this->itemMap.find(name_));

        if (existIt == this->itemMap.end()) // insert new item
        {
            NewItemFunc newFunc(newIt.second.second);
            this->itemMap[name_] = newFunc(data);
        }
        else existIt->second->Refresh(data); // update existing
    }

    ItemMap::const_iterator oldIt = this->itemMap.begin();
    for (; oldIt != this->itemMap.end();)
    {
        if (newItems.find(oldIt->first) == newItems.end())
        {
            oldIt = this->itemMap.erase(oldIt); // deleted on server
        }
        else ++oldIt;
    }
}

/*****************************************************/
void Folder::CreateFile(const std::string& name_)
{
    debug << this->name << ":" << __func__ << "(name:" << name_ << ")"; debug.Info();

    const ItemMap& items = GetItems(); // pre-populate items

    if (items.count(name_)) throw DuplicateItemException();

    SubCreateFile(name_);
}

/*****************************************************/
void Folder::CreateFolder(const std::string& name_)
{
    debug << this->name << ":" << __func__ << "(name:" << name_ << ")"; debug.Info();

    const ItemMap& items = GetItems(); // pre-populate items

    if (items.count(name_)) throw DuplicateItemException();

    SubCreateFolder(name_);
}

/*****************************************************/
void Folder::DeleteItem(const std::string& name_)
{
    debug << this->name << ":" << __func__ << "(name:" << name_ << ")"; debug.Info();

    GetItems(); ItemMap::const_iterator it = this->itemMap.find(name_);
    if (it == this->itemMap.end()) throw NotFoundException();

    SubDeleteItem(*(it->second)); this->itemMap.erase(it);
}

/*****************************************************/
void Folder::RenameItem(const std::string& oldName, const std::string& newName, bool overwrite)
{
    debug << this->name << ":" << __func__ << "(oldName:" << oldName << " newName:" << newName << ")"; debug.Info();

    GetItems(); ItemMap::const_iterator it = this->itemMap.find(oldName);
    if (it == this->itemMap.end()) throw NotFoundException();

    ItemMap::const_iterator dup = this->itemMap.find(newName);
    if (!overwrite && dup != this->itemMap.end())
        throw DuplicateItemException();

    SubRenameItem(*(it->second), newName, overwrite);

    if (dup != this->itemMap.end()) this->itemMap.erase(dup);

    ItemMap::node_type node(this->itemMap.extract(it));
    node.key() = newName; this->itemMap.insert(std::move(node));
}

/*****************************************************/
void Folder::MoveItem(const std::string& name_, Folder& newParent, bool overwrite)
{
    debug << this->name << ":" << __func__ << "(name:" << name_ << " parent:" << newParent.GetName() << ")"; debug.Info();

    GetItems(); ItemMap::const_iterator it = this->itemMap.find(name_);
    if (it == this->itemMap.end()) throw NotFoundException();

    newParent.GetItems(); if (newParent.isReadOnly()) throw ModifyException();

    ItemMap::const_iterator dup = newParent.itemMap.find(name_);    
    if (!overwrite && dup != newParent.itemMap.end())
        throw DuplicateItemException();

    SubMoveItem(*(it->second), newParent, overwrite);

    if (dup != newParent.itemMap.end()) newParent.itemMap.erase(dup);

    newParent.itemMap.insert(this->itemMap.extract(it));
}

/*****************************************************/
void Folder::FlushCache(bool nothrow)
{
    for (ItemMap::value_type& it : itemMap)
        it.second->FlushCache(nothrow);
}

} // namespace FSItems
} // namespace Andromeda
