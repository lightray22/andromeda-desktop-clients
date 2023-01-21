#include <utility>
#include <nlohmann/json.hpp>

#include "Folder.hpp"
#include "File.hpp"
#include "andromeda/Utilities.hpp"
using Andromeda::Utilities;
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/backend/ConfigOptions.hpp"
using Andromeda::Backend::ConfigOptions;
#include "andromeda/filesystem/folders/PlainFolder.hpp"
using Andromeda::Filesystem::Folders::PlainFolder;

using namespace std::chrono;

namespace Andromeda {
namespace Filesystem {

/*****************************************************/
Folder::Folder(BackendImpl& backend) : 
    Item(backend), mDebug("Folder",this)
{
    MDBG_INFO("()");
}

/*****************************************************/
Item& Folder::GetItemByPath(std::string path)
{
    ITDBG_INFO("(path:" << path << ")");

    if (path[0] == '/') path.erase(0,1);

    if (path.empty()) return *this;    

    Utilities::StringList parts { Utilities::explode(path,"/") };

    // iteratively (not recursively) find the correct parent/subitem
    Folder* parent { this }; 
    for (Utilities::StringList::iterator pIt { parts.begin() }; 
        pIt != parts.end(); ++pIt )
    {
        const ItemMap& items { parent->GetItems() };
        ItemMap::const_iterator it { items.find(*pIt) };
        if (it == items.end()) throw NotFoundException();

        Item& item { *(it->second) };

        if (std::next(pIt) == parts.end()) return item; // last part of path

        if (item.GetType() != Type::FOLDER) throw NotFolderException();

        parent = dynamic_cast<Folder*>(&item);
    }

    throw NotFoundException(); // should never get here
}

/*****************************************************/
File& Folder::GetFileByPath(const std::string& path)
{
    Item& item { GetItemByPath(path) };

    if (item.GetType() != Type::FILE)
        throw NotFileException();

    return dynamic_cast<File&>(item);
}

/*****************************************************/
Folder& Folder::GetFolderByPath(const std::string& path)
{
    Item& item { GetItemByPath(path) };

    if (item.GetType() != Type::FOLDER)
        throw NotFolderException();

    return dynamic_cast<Folder&>(item);
}

/*****************************************************/
const Folder::ItemMap& Folder::GetItems()
{
    bool expired { (steady_clock::now() - mRefreshed)
        > mBackend.GetOptions().refreshTime };

    if (!mHaveItems || (expired && !mBackend.isMemory()) ||
        mBackend.GetOptions().cacheType == ConfigOptions::CacheType::NONE) 
    {
        LoadItems(); mRefreshed = steady_clock::now();
    }

    mHaveItems = true;
    return mItemMap;
}

/*****************************************************/
void Folder::LoadItemsFrom(const nlohmann::json& data)
{
    ITDBG_INFO("()");

    Folder::NewItemMap newItems;

    NewItemFunc newFile { [&](const nlohmann::json& fileJ)->std::unique_ptr<Item> {
        return std::make_unique<File>(mBackend, fileJ, *this); } };

    NewItemFunc newFolder { [&](const nlohmann::json& folderJ)->std::unique_ptr<Item> {
        return std::make_unique<Folders::PlainFolder>(mBackend, &folderJ, this); } };

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
        throw BackendImpl::JSONErrorException(ex.what()); }

    SyncContents(newItems);

    mHaveItems = true;
    mRefreshed = steady_clock::now();
}

/*****************************************************/
void Folder::SyncContents(const Folder::NewItemMap& newItems)
{
    ITDBG_INFO("()");

    for (const NewItemMap::value_type& newIt : newItems)
    {
        const std::string& name(newIt.first);
        const nlohmann::json& data(newIt.second.first);

        ItemMap::const_iterator existIt(mItemMap.find(name));

        if (existIt == mItemMap.end()) // insert new item
        {
            NewItemFunc newFunc(newIt.second.second);
            mItemMap[name] = newFunc(data);
        }
        else existIt->second->Refresh(data); // update existing
    }

    ItemMap::const_iterator oldIt { mItemMap.begin() };
    for (; oldIt != mItemMap.end();)
    {
        if (newItems.find(oldIt->first) == newItems.end())
        {
            oldIt = mItemMap.erase(oldIt); // deleted on server
            ITDBG_INFO("... remote deleted: " << oldIt->second->GetName());
        }
        else ++oldIt;
    }
}

/*****************************************************/
void Folder::CreateFile(const std::string& name)
{
    ITDBG_INFO("(name:" << name << ")");

    const ItemMap& items { GetItems() }; // pre-populate items

    if (items.count(name)) throw DuplicateItemException();

    SubCreateFile(name);
}

/*****************************************************/
void Folder::CreateFolder(const std::string& name)
{
    ITDBG_INFO("(name:" << name << ")");

    const ItemMap& items { GetItems() }; // pre-populate items

    if (items.count(name)) throw DuplicateItemException();

    SubCreateFolder(name);
}

/*****************************************************/
void Folder::DeleteItem(const std::string& name)
{
    ITDBG_INFO("(name:" << name << ")");

    GetItems(); ItemMap::const_iterator it { mItemMap.find(name) };
    if (it == mItemMap.end()) throw NotFoundException();

    SubDeleteItem(*(it->second)); mItemMap.erase(it);
}

/*****************************************************/
void Folder::RenameItem(const std::string& oldName, const std::string& newName, bool overwrite)
{
    ITDBG_INFO("(oldName:" << oldName << " newName:" << newName << ")");

    GetItems(); ItemMap::const_iterator it { mItemMap.find(oldName) };
    if (it == mItemMap.end()) throw NotFoundException();

    ItemMap::const_iterator dup { mItemMap.find(newName) };
    if (!overwrite && dup != mItemMap.end())
        throw DuplicateItemException();

    SubRenameItem(*(it->second), newName, overwrite);

    if (dup != mItemMap.end()) mItemMap.erase(dup);

    ItemMap::node_type node(mItemMap.extract(it));
    node.key() = newName; mItemMap.insert(std::move(node));
}

/*****************************************************/
void Folder::MoveItem(const std::string& name, Folder& newParent, bool overwrite)
{
    ITDBG_INFO("(name:" << name << " parent:" << newParent.GetName() << ")");

    GetItems(); ItemMap::const_iterator it { mItemMap.find(name) };
    if (it == mItemMap.end()) throw NotFoundException();

    newParent.GetItems(); if (newParent.isReadOnly()) throw ModifyException();

    ItemMap::const_iterator dup { newParent.mItemMap.find(name) };
    if (!overwrite && dup != newParent.mItemMap.end())
        throw DuplicateItemException();

    SubMoveItem(*(it->second), newParent, overwrite);

    if (dup != newParent.mItemMap.end()) newParent.mItemMap.erase(dup);

    newParent.mItemMap.insert(mItemMap.extract(it));
}

/*****************************************************/
void Folder::FlushCache(bool nothrow)
{
    for (ItemMap::value_type& it : mItemMap)
        it.second->FlushCache(nothrow);
}

} // namespace Filesystem
} // namespace Andromeda
