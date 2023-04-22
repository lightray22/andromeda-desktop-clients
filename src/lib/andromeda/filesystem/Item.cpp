
#include <nlohmann/json.hpp>

#include "Item.hpp"
#include "Folder.hpp"
#include "FSConfig.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;

namespace Andromeda {
namespace Filesystem {

/*****************************************************/
Item::Item(BackendImpl& backend) : 
    mBackend(backend), mDebug(__func__,this)
{
    MDBG_INFO("()");
}

/*****************************************************/
Item::Item(BackendImpl& backend, const nlohmann::json& data) : 
    mBackend(backend), mDebug(__func__,this)
{
    MDBG_INFO("()");

    try
    {
        data.at("id").get_to(mId);
        data.at("name").get_to(mName);

        if (data.contains("dates"))
        {
            data.at("dates").at("created").get_to(mCreated);
            
            const nlohmann::json& modifiedJ(data.at("dates").at("modified"));
            if (!modifiedJ.is_null()) modifiedJ.get_to(mModified);

            const nlohmann::json& accessedJ(data.at("dates").at("accessed"));
            if (!accessedJ.is_null()) accessedJ.get_to(mAccessed);
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

/*****************************************************/
void Item::Refresh(const nlohmann::json& data, const SharedLockW& thisLock)
{
    ITDBG_INFO("()");

    try
    {
        decltype(mName) newName; 
        data.at("name").get_to(newName);
        if (newName != mName)
        {
            mName = newName;
            ITDBG_INFO("... newName:" << mName);
        }
        
        decltype(mCreated) newCreated; 
        data.at("dates").at("created").get_to(newCreated);
        if (newCreated != mCreated)
        {
            mCreated = newCreated;
            ITDBG_INFO("... newCreated:" << mCreated);
        }

        const nlohmann::json& modifiedJ(data.at("dates").at("modified"));
        if (!modifiedJ.is_null())
        {
            decltype(mModified) newModified; 
            modifiedJ.get_to(newModified);
            if (newModified != mModified)
            {
                mModified = newModified;
                ITDBG_INFO("... newModified:" << mModified);
            }
        }

        const nlohmann::json& accessedJ(data.at("dates").at("accessed"));
        if (!accessedJ.is_null())
        {
            decltype(mAccessed) newAccessed; 
            accessedJ.get_to(newAccessed);
            if (newAccessed != mAccessed)
            {
                mAccessed = newAccessed;
                ITDBG_INFO("... newAccessed:" << mAccessed);
            }
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

/*****************************************************/
Folder& Item::GetParent(const SharedLock& thisLock) const
{
    if (mParent == nullptr) 
        throw NullParentException();
            
    return *mParent; 
}

/*****************************************************/
const FSConfig& Item::GetFSConfig() const
{
    if (mFsConfig == nullptr)
        throw NullFSConfigException();

    return *mFsConfig;
}

/*****************************************************/
bool Item::isReadOnlyFS() const
{
    bool retval { mBackend.isReadOnly() };

    if (HasFSConfig()) retval |= GetFSConfig().isReadOnly();

    return retval;
}

/*****************************************************/
void Item::DeleteSelf(DeleteLock& deleteLock, const SharedLockW& thisLock)
{
    ITDBG_INFO("()");

    if (HasParent(thisLock))
        throw HasParentException();

    SubDelete(deleteLock);
    deleteLock.MarkDeleted();
}

/*****************************************************/
void Item::Delete(Item::ScopeLocked& scopeLock, SharedLockW& thisLock)
{
    ITDBG_INFO("()");

    // Deleting the item involves grabbing its scopeLock exclusively, so we need to unlock our scopeLock.
    // But first, we need to get the parent's scopeLock since we no longer have the protection
    // where the parent can't go out of scope while any children's scopeLocks are held.

    // The delete must be done through the parent.  Get the parent, unlock self, lock parent.
    // This opens a window where the item could be deleted elsewhere - will cause NotFoundException
    Folder::ScopeLocked parent { GetParent(thisLock).TryLockScope() };
    if (!parent) throw Folder::NotFoundException();

    thisLock.unlock();
    scopeLock.unlock();

    SharedLockW parentLock { parent->GetWriteLock() };
    parent->DeleteItem(mName, parentLock);
}

/*****************************************************/
void Item::RenameSelf(const std::string& newName, const SharedLockW& thisLock, bool overwrite)
{
    ITDBG_INFO("(newName:" << newName << ")");

    if (HasParent(thisLock))
        throw HasParentException();

    SubRename(newName, thisLock, overwrite);
}

/*****************************************************/
void Item::Rename(const std::string& newName, SharedLockW& thisLock, bool overwrite)
{
    ITDBG_INFO("(newName:" << newName << ")");

    // The rename must be done through the parent.  Get the parent, unlock self, lock parent.
    // This opens a window where the item could be renamed elsewhere - will cause NotFoundException
    Folder& parent { GetParent(thisLock) };
    thisLock.unlock();

    // need a lock on the parent, not a lock on us
    SharedLockW parentLock { parent.GetWriteLock() };
    parent.RenameItem(mName, newName, parentLock, overwrite);
    mName = newName;
}

/*****************************************************/
void Item::Move(Folder& newParent, SharedLockW& thisLock, bool overwrite)
{
    ITDBG_INFO("(newParent:" << newParent.GetID() << ")");

    // The move must be done through the parent.  Get the parent, unlock self, lock parent.
    // This opens a window where the item could be moved elsewhere - will cause NotFoundException
    Folder& parent { GetParent(thisLock) };
    thisLock.unlock();

    // need a lock on the parent, not a lock on us
    SharedLockW::LockPair parentLocks { parent.GetWriteLockPair(newParent) };
    parent.MoveItem(mName, newParent, parentLocks, overwrite);
    mParent = &newParent;
}

} // namespace Filesystem
} // namespace Andromeda
