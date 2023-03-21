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
Item::ScopeLocked Folder::GetItemByPath(std::string path, const SharedLock& itemLock)
{
    ITDBG_INFO("(path:" << path << ")");

    while (path[0] == '/') path.erase(0,1);

    if (path.empty()) // return self
    {
        Item::ScopeLocked self { Item::TryLockScope() };
        if (self) return self; else throw NotFoundException();
    }

    Utilities::StringList parts { Utilities::explode(path,"/") };

    // iteratively find the correct parent/subitem
    Folder::ScopeLocked parent { TryLockScope() };
    if (!parent) throw NotFoundException(); // was deleted

    for (Utilities::StringList::iterator pIt { parts.begin() }; 
        pIt != parts.end(); ++pIt )
    {
        parent->LoadItems(); // populate items
        const ItemMap& items { parent->mItemMap };

        ItemMap::const_iterator it { items.find(*pIt) };
        if (it == items.end()) throw NotFoundException();

        Item::ScopeLocked item { it->second->TryLockScope() };
        if (!item) throw NotFoundException(); // was deleted

        if (std::next(pIt) == parts.end()) return item; // last part of path

        if (item->GetType() != Type::FOLDER) throw NotFolderException();
        parent = ScopeLocked::FromBase(std::move(item));
    }

    throw NotFoundException(); // should never get here
}

/*****************************************************/
File::ScopeLocked Folder::GetFileByPath(const std::string& path, const SharedLock& itemLock)
{
    Item::ScopeLocked item { GetItemByPath(path, itemLock) };

    if (item->GetType() != Type::FILE)
        throw NotFileException();

    return File::ScopeLocked::FromBase(std::move(item));
}

/*****************************************************/
Folder::ScopeLocked Folder::GetFolderByPath(const std::string& path, const SharedLock& itemLock)
{
    Item::ScopeLocked item { GetItemByPath(path, itemLock) };

    if (item->GetType() != Type::FOLDER)
        throw NotFolderException();

    return Folder::ScopeLocked::FromBase(std::move(item));
}

/*****************************************************/
Folder::LockedItemMap Folder::GetItems(const SharedLock& itemLock)
{
    LoadItems(); // populate

    Folder::LockedItemMap itemMap;
    for (decltype(mItemMap)::value_type& it : mItemMap)
    {
        // don't check lock, can't go out of scope with itemMap lock
        itemMap[it.first] = it.second->TryLockScope();
    }

    return itemMap;
}

/*****************************************************/
size_t Folder::CountItems(const SharedLock& itemLock)
{
    LoadItems(); // populate
    return mItemMap.size();
}

/*****************************************************/
void Folder::LoadItems()
{
    bool expired { (steady_clock::now() - mRefreshed)
        > mBackend.GetOptions().refreshTime };

    ITDBG_INFO("(expired:" << (expired?"true":"false") << ")");

    if (!mHaveItems || (expired && !mBackend.isMemory()))
    {
        SubLoadItems(); // populate mItemMap
        mRefreshed = steady_clock::now();
        mHaveItems = true;
    }
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
            const SharedLockR itLock { oldIt->second->GetReadLock() };

            if (oldIt->second->GetType() != Type::FILE ||
                dynamic_cast<const File&>(*oldIt->second).ExistsOnBackend(itLock))
            {
                ITDBG_INFO("... remote deleted: " << oldIt->second->GetName(itLock));
                oldIt = mItemMap.erase(oldIt); // deleted on server
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

    LoadItems(); // populate items

    if (mItemMap.count(name) || name.empty()) 
        throw DuplicateItemException();

    SubCreateFile(name, itemLock);
}

/*****************************************************/
void Folder::CreateFolder(const std::string& name, const SharedLockW& itemLock)
{
    ITDBG_INFO("(name:" << name << ")");

    LoadItems(); // populate items

    if (mItemMap.count(name) || name.empty()) 
        throw DuplicateItemException();

    SubCreateFolder(name, itemLock);
}

/*****************************************************/
void Folder::DeleteItem(const std::string& name, const SharedLockW& itemLock)
{
    ITDBG_INFO("(name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    LoadItems(); // populate items
    ItemMap::const_iterator it { mItemMap.find(name) };
    if (it == mItemMap.end()) throw NotFoundException();

    SharedLockW subLock { it->second->GetWriteLock() };
    it->second->SubDelete(subLock); 
    mItemMap.erase(it);
}

/*****************************************************/
void Folder::RenameItem(const std::string& oldName, const std::string& newName, const SharedLockW& itemLock, bool overwrite)
{
    ITDBG_INFO("(oldName:" << oldName << " newName:" << newName << ")");

    if (isReadOnly()) throw ReadOnlyException();

    LoadItems(); // populate items
    ItemMap::const_iterator it { mItemMap.find(oldName) };
    if (it == mItemMap.end()) throw NotFoundException();

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
void Folder::MoveItem(const std::string& name, Folder& newParent, const SharedLockW& itemLock, bool overwrite)
{
    ITDBG_INFO("(name:" << name << " parent:" << newParent.GetID() << ")");

    if (isReadOnly()) throw ReadOnlyException();

    LoadItems(); // populate items
    ItemMap::const_iterator it { mItemMap.find(name) };
    if (it == mItemMap.end()) throw NotFoundException();

    newParent.LoadItems(); // populate items
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
