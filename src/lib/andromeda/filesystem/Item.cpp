
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
Item::~Item()
{
    MDBG_INFO("() waiting for lock");

    mScopeMutex.lock(); // exclusive, forever

    MDBG_INFO("... returning!");
}

/*****************************************************/
void Item::Refresh(const nlohmann::json& data, const Andromeda::SharedLockW& itemLock)
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
Folder& Item::GetParent(const Andromeda::SharedLock& itemLock) const
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
bool Item::isReadOnly() const
{
    bool retval { mBackend.isReadOnly() };

    if (HasFSConfig()) retval |= GetFSConfig().isReadOnly();

    return retval;
}

/*****************************************************/
void Item::Delete(Item::ScopeLocked& scopeLock, SharedLockW& itemLock)
{
    ITDBG_INFO("()");

    if (!HasParent(itemLock)) 
        SubDelete(itemLock);
    else
    {
        // deleting the item involves grabbing its scopeLock exclusively, so we need to unlock our scopeLock
        // before doing that, we need to get the parent's scopeLock though since we no longer have the protection
        // where the parent can't go out of scope while any children's scopeLocks are held.  This does introduce
        // a small window where someone could concurrently delete the file before us, but if that happens we'll just
        // throw a NotFoundException when we tell the parent to delete, which is fine.  A child reaching into its
        // parent to lock is backwards and hints at deadlock but really is okay because it's a shared lock.
        Folder::ScopeLocked parent { GetParent(itemLock).TryLockScope() };
        scopeLock.unlock();
        itemLock.unlock();

        SharedLockW parentLock { parent->GetWriteLock() };
        parent->DeleteItem(mName, parentLock);
    }
}

/*****************************************************/
void Item::Rename(const std::string& newName, SharedLockW& itemLock, bool overwrite)
{
    ITDBG_INFO("(newName:" << newName << ")");

    if (!HasParent(itemLock)) 
        SubRename(newName, itemLock, overwrite);
    else
    {
        Folder& parent { GetParent(itemLock) };
        itemLock.unlock();

        SharedLockW parentLock { parent.GetWriteLock() };
        parent.RenameItem(mName, newName, parentLock, overwrite);
        mName = newName;
    }
}

/*****************************************************/
void Item::Move(Folder& newParent, SharedLockW& itemLock, bool overwrite)
{
    ITDBG_INFO("(newParent:" << newParent.GetID() << ")");

    // GetParent() will throw if parent is null - MUST have parent!
    Folder& parent { GetParent(itemLock) };
    itemLock.unlock();

    SharedLockW parentLock { parent.GetWriteLock() };
    parent.MoveItem(mName, newParent, parentLock, overwrite);
    mParent = &newParent;
}

} // namespace Filesystem
} // namespace Andromeda
