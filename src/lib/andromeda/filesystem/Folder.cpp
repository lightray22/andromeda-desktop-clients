#include <utility>
#include <nlohmann/json.hpp>

#include "Folder.hpp"
#include "andromeda/ConfigOptions.hpp"
using Andromeda::ConfigOptions;
#include "andromeda/Utilities.hpp"
using Andromeda::Utilities;
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;

using namespace std::chrono;

namespace Andromeda {
namespace Filesystem {

/*****************************************************/
Folder::Folder(BackendImpl& backend) : 
    Item(backend), mDebug(__func__,this)
{
    MDBG_INFO("()");
}

/*****************************************************/
Folder::Folder(BackendImpl& backend, const nlohmann::json& data) : 
    Item(backend, data), mDebug(__func__,this)
{
    MDBG_INFO("()");
}

/*****************************************************/
Item::ScopeLocked Folder::GetItemByPath(std::string path)
{
    ITDBG_INFO("(path:" << path << ")");

    while (path[0] == '/') path.erase(0,1);

    if (path.empty()) // return self
    {
        Item::ScopeLocked self { Item::TryLockScope() };
        if (!self) { ITDBG_INFO("... self deleted(1)"); 
            throw NotFoundException(); }
        return Item::TryLockScope();
    }

    Utilities::StringList parts { Utilities::explode(path,"/") };

    // iteratively find the correct parent/subitem
    Folder::ScopeLocked parent { TryLockScope() };
    if (!parent) { ITDBG_INFO("... self deleted(2)"); 
        throw NotFoundException(); }

    for (Utilities::StringList::iterator pIt { parts.begin() }; 
        pIt != parts.end(); ++pIt )
    {
        const SharedLockW parentLock { parent->GetWriteLock() };
        parent->LoadItems(parentLock); // populate items
        const ItemMap& items { parent->mItemMap };

        ItemMap::const_iterator it { items.find(*pIt) };
        if (it == items.end()) {
            ITDBG_INFO("... not in map: " << *pIt); 
            throw NotFoundException(); }

        Item::ScopeLocked item { it->second->TryLockScope() };
        if (!item) { ITDBG_INFO("... item deleted: " << *pIt); 
            throw NotFoundException(); }

        if (std::next(pIt) == parts.end()) return item; // last part of path

        if (item->GetType() != Type::FOLDER) throw NotFolderException();
        parent = ScopeLocked::FromBase(std::move(item));
    }

    ITDBG_ERROR("... PAST LOOP!");
    throw NotFoundException(); // should never get here
}

/*****************************************************/
File::ScopeLocked Folder::GetFileByPath(const std::string& path)
{
    Item::ScopeLocked item { GetItemByPath(path) };

    if (item->GetType() != Type::FILE)
        throw NotFileException();

    return File::ScopeLocked::FromBase(std::move(item));
}

/*****************************************************/
Folder::ScopeLocked Folder::GetFolderByPath(const std::string& path)
{
    Item::ScopeLocked item { GetItemByPath(path) };

    if (item->GetType() != Type::FOLDER)
        throw NotFolderException();

    return Folder::ScopeLocked::FromBase(std::move(item));
}

/*****************************************************/
Folder::LockedItemMap Folder::GetItems(const SharedLockW& itemLock)
{
    LoadItems(itemLock); // populate

    Folder::LockedItemMap itemMap;
    for (decltype(mItemMap)::value_type& it : mItemMap)
    {
        // don't check lock, can't go out of scope with map lock
        itemMap[it.first] = it.second->TryLockScope();
    }

    return itemMap;
}

/*****************************************************/
size_t Folder::CountItems(const SharedLockW& itemLock)
{
    LoadItems(itemLock); // populate
    return mItemMap.size();
}

/*****************************************************/
void Folder::LoadItems(const SharedLockW& itemLock, bool force)
{
    bool expired { (steady_clock::now() - mRefreshed)
        > mBackend.GetOptions().refreshTime };

    if (force || !mHaveItems || (expired && !mBackend.isMemory()))
    {
        ITDBG_INFO("... expired!");

        ItemLockMap lockMap; // get locks for all existing items to prevent backend changes
        for (const ItemMap::value_type& it : mItemMap)
            lockMap.emplace(it.first, it.second->GetReadLock());
        // item scope lock not needed since mItemMap is locked

        SubLoadItems(lockMap, itemLock); // populate mItemMap
        mRefreshed = steady_clock::now();
        mHaveItems = true;
    }
}

/*****************************************************/
void Folder::SyncContents(const NewItemMap& newItems, ItemLockMap& itemsLocks, const SharedLockW& itemLock)
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
        else 
        {
            const SharedLockW itLock { existIt->second->GetWriteLock() };
            existIt->second->Refresh(data, itLock); // update existing
        }
    }

    ItemMap::const_iterator oldIt { mItemMap.begin() };
    for (; oldIt != mItemMap.end();)
    {
        if (newItems.find(oldIt->first) == newItems.end())
        {
            SharedLockR& itLock { itemsLocks.at(oldIt->first) };

            if (oldIt->second->GetType() != Type::FILE ||
                dynamic_cast<const File&>(*oldIt->second).ExistsOnBackend(itLock))
            {
                ITDBG_INFO("... remote deleted: " << oldIt->second->GetName(itLock));
                itLock.unlock(); // scope locks come first

                DeleteLock deleteLock { oldIt->second->GetDeleteLock() };
                deleteLock.MarkDeleted();
                oldIt = mItemMap.erase(oldIt);
            }
            else ++oldIt;
        }
        else ++oldIt;
    }
}

/*****************************************************/
void Folder::CreateFile(const std::string& name, const SharedLockW& itemLock)
{
    ITDBG_INFO("(name:" << name << ")");

    LoadItems(itemLock); // populate items

    if (mItemMap.count(name) || name.empty()) 
        throw DuplicateItemException();

    SubCreateFile(name, itemLock);
}

/*****************************************************/
void Folder::CreateFolder(const std::string& name, const SharedLockW& itemLock)
{
    ITDBG_INFO("(name:" << name << ")");

    LoadItems(itemLock); // populate items

    if (mItemMap.count(name) || name.empty()) 
        throw DuplicateItemException();

    SubCreateFolder(name, itemLock);
}

/*****************************************************/
void Folder::DeleteItem(const std::string& name, const SharedLockW& itemLock)
{
    ITDBG_INFO("(name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    LoadItems(itemLock); // populate items
    ItemMap::const_iterator it { mItemMap.find(name) };
    if (it == mItemMap.end()) throw NotFoundException();

    DeleteLock deleteLock { it->second->GetDeleteLock() };
    it->second->SubDelete(deleteLock);
    deleteLock.MarkDeleted();
    mItemMap.erase(it);
}

/*****************************************************/
void Folder::RenameItem(const std::string& oldName, const std::string& newName, const SharedLockW& itemLock, bool overwrite)
{
    ITDBG_INFO("(oldName:" << oldName << " newName:" << newName << ")");

    if (isReadOnly()) throw ReadOnlyException();

    LoadItems(itemLock); // populate items
    ItemMap::const_iterator it { mItemMap.find(oldName) };
    if (it == mItemMap.end()) throw NotFoundException();
    // item scope lock not needed since mItemMap is locked

    ItemMap::const_iterator dup { mItemMap.find(newName) };
    if ((!overwrite && dup != mItemMap.end()) || newName.empty())
        throw DuplicateItemException();

    SharedLockW subLock { it->second->GetWriteLock() };
    it->second->SubRename(newName, subLock, overwrite);

    if (dup != mItemMap.end()) 
        mItemMap.erase(dup);

    ItemMap::node_type node(mItemMap.extract(it));
    node.key() = newName; mItemMap.insert(std::move(node));
}

/*****************************************************/
void Folder::MoveItem(const std::string& name, Folder& newParent, const SharedLockW::LockPair& itemLocks, bool overwrite)
{
    ITDBG_INFO("(name:" << name << " parent:" << newParent.GetID() << ")");

    if (isReadOnly()) throw ReadOnlyException();

    LoadItems(itemLocks.first); // populate items
    ItemMap::const_iterator it { mItemMap.find(name) };
    if (it == mItemMap.end()) throw NotFoundException();
    // item scope lock not needed since mItemMap is locked

    newParent.LoadItems(itemLocks.second); // populate items
    if (newParent.isReadOnly()) throw ReadOnlyException();
    if (newParent.GetID().empty()) throw ModifyException();

    ItemMap::const_iterator dup { newParent.mItemMap.find(name) };
    if (!overwrite && dup != newParent.mItemMap.end())
        throw DuplicateItemException();

    SharedLockW subLock { it->second->GetWriteLock() };
    it->second->SubMove(newParent.GetID(), subLock, overwrite);

    if (dup != newParent.mItemMap.end()) 
        newParent.mItemMap.erase(dup);

    newParent.mItemMap.insert(mItemMap.extract(it));
}

/*****************************************************/
void Folder::FlushCache(const SharedLockW& itemLock, bool nothrow)
{
    for (ItemMap::value_type& it : mItemMap)
    {
        SharedLockW subLock { it.second->GetWriteLock() };
        it.second->FlushCache(subLock, nothrow);
    }
}

} // namespace Filesystem
} // namespace Andromeda
