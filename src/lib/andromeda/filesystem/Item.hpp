#ifndef LIBA2_ITEM_H_
#define LIBA2_ITEM_H_

#include <shared_mutex>
#include <string>

#include <nlohmann/json_fwd.hpp>

#include "andromeda/BaseException.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/ScopeLocked.hpp"
#include "andromeda/SharedMutex.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {
class FSConfig;

class Folder;

/** An abstract item in a filesystem */
class Item
{
public:

    class Exception : public BaseException { public:
        using BaseException::BaseException; };
    
    /** Exception indicating this item has no parent */
    class NullParentException : public Exception { public:
        NullParentException() : Exception("Null Parent") {}; };

    /** Exception indicating the item has no filesystem */
    class NullFSConfigException : public Exception { public:
        NullFSConfigException() : Exception("Null FSConfig") {}; };

    /** Exception indicating the item is read-only */
    class ReadOnlyException : public Exception { public:
        ReadOnlyException() : Exception("Read Only Backend") {}; };

    /** Macro to print the file name at the beginning of debug */
    #define ITDBG_INFO(strfunc) MDBG_INFO("(" << mName << ")" << strfunc)

    /** Macro to print the file name at the beginning of debug */
    #define ITDBG_ERROR(strfunc) MDBG_ERROR("(" << mName << ")" << strfunc)

    typedef Andromeda::ScopeLocked<Item> ScopeLocked;
    /** 
     * Tries to lock mScopeMutex, returns a ref that is maybe locked 
     * The purpose of scope locking is to make sure the item isn't removed while being used
     */
    inline ScopeLocked TryLockScope() { return ScopeLocked(*this, mScopeMutex); }

    /** Returns a read lock for this item */
    inline SharedLockR GetReadLock() const { return SharedLockR(mItemMutex); }
    
    /** Returns a priority read lock for this item */
    inline SharedLockRP GetReadPriLock() const { return SharedLockRP(mItemMutex); }
    
    /** Returns a write lock for this item */
    inline SharedLockW GetWriteLock() { return SharedLockW(mItemMutex); }

    virtual ~Item();

    /** API date format */
    typedef double Date;

    /** Concrete item types */
    enum class Type { FILE, FOLDER };

    /** Returns the FS type */
    virtual Type GetType() const = 0;

    /** Returns a reference to the backend for this item */
    virtual Backend::BackendImpl& GetBackend() const final { return mBackend; }

    /** Returns true if this item has a parent */
    virtual bool HasParent(const Andromeda::SharedLock& itemLock) const { return mParent != nullptr; }

    /** Returns the parent folder */
    virtual Folder& GetParent(const Andromeda::SharedLock& itemLock) const;

    /** Returns true if this item has FSconfig */
    virtual bool HasFSConfig() const { return mFsConfig != nullptr; }

    /** Returns the filesystem config */
    virtual const FSConfig& GetFSConfig() const;

    /** Returns the item's name */
    virtual const std::string& GetName(const Andromeda::SharedLock& itemLock) const final { return mName; }

    /** Get the created time stamp */
    virtual Date GetCreated(const Andromeda::SharedLock& itemLock) const final { return mCreated; }

    /** Get the modified time stamp */
    virtual Date GetModified(const Andromeda::SharedLock& itemLock) const final { return mModified; }

    /** Get the accessed time stamp */
    virtual Date GetAccessed(const Andromeda::SharedLock& itemLock) const final { return mAccessed; }

    /** Returns true if the item is read-only */
    virtual bool isReadOnly() const;

    /** Refresh the item given updated server JSON data */
    virtual void Refresh(const nlohmann::json& data, const Andromeda::SharedLockW& itemLock);

    /** 
     * Delete this item (and its contents if a folder) 
     * @param scopeLock reference to scopeLock which will be unlocked
     */
    virtual void Delete(ScopeLocked& scopeLock, Andromeda::SharedLockW& itemLock) final;

    /** Set this item's name to the given name, optionally overwrite existing */
    virtual void Rename(const std::string& newName, Andromeda::SharedLockW& itemLock, bool overwrite = false) final;

    /** 
     * Move this item to the given parent folder, optionally overwrite existing 
     * REQUIRES that the item currently has a parent
     */
    virtual void Move(Folder& newParent, Andromeda::SharedLockW& itemLock, bool overwrite = false) final;

    /** 
     * Flushes all dirty pages and metadata to the backend
     * @param nothrow if true, no exceptions are thrown
     */
    virtual void FlushCache(const Andromeda::SharedLockW& itemLock, bool nothrow = false) = 0;

protected:

    /** 
     * Construct a new item
     * @param backend reference to backend
     */
    Item(Backend::BackendImpl& backend);

    /** Initialize from the given JSON data */
    Item(Backend::BackendImpl& backend, const nlohmann::json& data);

    /** Returns the Andromeda object ID */
    virtual const std::string& GetID() { return mId; }

    friend class Folder; // calls SubDelete(), SubRename(), SubMove()

    /** Item type-specific delete */
    virtual void SubDelete(const Andromeda::SharedLockW& itemLock) = 0;

    /** Item type-specific rename */
    virtual void SubRename(const std::string& newName, const Andromeda::SharedLockW& itemLock, bool overwrite) = 0;

    /** Item type-specific move */
    virtual void SubMove(const std::string& parentID, const Andromeda::SharedLockW& itemLock, bool overwrite) = 0;

    /** Reference to the API backend */
    Backend::BackendImpl& mBackend;

    /** Pointer to parent folder */
    Folder* mParent { nullptr };

    /** Pointer to filesystem config */
    const FSConfig* mFsConfig { nullptr };

    /** Backend object ID */
    std::string mId;

    /** Name of the item */
    std::string mName;

    /** Item creation timestamp */
    Date mCreated { 0 };

    /** Item modified timestamp */
    Date mModified { 0 };

    /** Item accessed timestamp */
    Date mAccessed { 0 };

    /** Shared mutex that is grabbed exclusively when this class is destructed */
    std::shared_mutex mScopeMutex;
    /** Primary shared mutex that protects this item */
    mutable SharedMutex mItemMutex;

private:

    Debug mDebug;

    Item(const Item&) = delete; // no copy
};

} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_ITEM_H_
