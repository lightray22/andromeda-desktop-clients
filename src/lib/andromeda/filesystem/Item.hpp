#ifndef LIBA2_ITEM_H_
#define LIBA2_ITEM_H_

#include <shared_mutex>
#include <string>

#include "nlohmann/json_fwd.hpp"

#include "andromeda/BaseException.hpp"
#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/ScopeLocked.hpp"
#include "andromeda/SharedMutex.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {
class FSConfig;

class Folder;

/** 
 * An abstract item object in a filesystem 
 * THREAD SAFE (FORCES EXTERNAL LOCKS) - use deleteLock + read/write lock
 */
class Item
{
public:

    class Exception : public BaseException { public:
        using BaseException::BaseException; };
    
    /** Exception indicating this item has no parent */
    class NullParentException : public Exception { public:
        NullParentException() : Exception("Null Parent") {}; };

    /** Exception indicating this item has a parent */
    class HasParentException : public Exception { public:
        HasParentException() : Exception("Non-null Parent") {}; };

    /** Exception indicating the item has no filesystem */
    class NullFSConfigException : public Exception { public:
        NullFSConfigException() : Exception("Null FSConfig") {}; };

    /** Exception indicating the item is read-only */
    class ReadOnlyFSException : public Exception { public:
        ReadOnlyFSException() : Exception("Read Only Backend") {}; };

    /** Exception indicating the item's name is invalid */
    class InvalidNameException : public Exception { public:
        InvalidNameException() : Exception("Invalid Item Name") {}; };

    /** Macro to print the file name at the beginning of debug */
    #define ITDBG_INFO(strfunc) MDBG_INFO("(" << mName << ")" << strfunc)

    /** Macro to print the file name at the beginning of debug */
    #define ITDBG_ERROR(strfunc) MDBG_ERROR("(" << mName << ")" << strfunc)

    using ScopeLocked = Andromeda::ScopeLocked<Item>;
    /** 
     * Tries to lock mScopeMutex, returns a ref that is maybe locked 
     * The purpose of scope locking is to make sure the item isn't removed while being used
     */
    inline ScopeLocked TryLockScope() { return ScopeLocked(*this, mScopeMutex); }

    using DeleteLock = std::unique_lock<std::shared_mutex>;
    /** Permanently, exclusively locks the scope lock if not acquired (use before deleting) */
    virtual DeleteLock GetDeleteLock() { return DeleteLock(mScopeMutex); }

    /** Returns a read lock for this item */
    inline SharedLockR GetReadLock() const { return SharedLockR(mItemMutex); }
    
    /** Returns a priority read lock for this item */
    inline SharedLockRP GetReadPriLock() const { return SharedLockRP(mItemMutex); }
    
    /** Returns a possibly-locked write lock for this item (check operator bool) */
    inline SharedLockW TryGetWriteLock() { return SharedLockW(mItemMutex,std::try_to_lock); }

    /** Returns a write lock for this item */
    inline SharedLockW GetWriteLock() { return SharedLockW(mItemMutex); }

    /** Returns a dual write lock for this item and another (deadlock safe) */
    inline SharedLockW::LockPair GetWriteLockPair(Item& item) { 
        return SharedLockW::get_pair(mItemMutex, item.mItemMutex); }

    virtual ~Item() = default;
    DELETE_COPY(Item)
    DELETE_MOVE(Item)

    /** API date format */
    using Date = double;

    /** Concrete item types */
    enum class Type : uint8_t { FILE, FOLDER };

    /** Returns the FS type */
    virtual Type GetType() const = 0;

    /** Returns a reference to the backend for this item */
    virtual Backend::BackendImpl& GetBackend() const final { return mBackend; }

    /** Returns true if this item has a parent */
    virtual bool HasParent(const SharedLock& thisLock) const { return mParent != nullptr; }

    /** 
     * Returns the parent folder
     * @throws NullParentException
     */
    virtual Folder& GetParent(const SharedLock& thisLock) const;

    /** Returns the parent folder or null if not set */
    virtual Folder* TryGetParent(const SharedLock& thisLock) const { return mParent; }

    /** Returns true if this item has FSconfig */
    virtual bool HasFSConfig() const { return mFsConfig != nullptr; }

    /** 
     * Returns the filesystem config
     * @throws NullFSConfigException
     */
    virtual const FSConfig& GetFSConfig() const;

    /** Returns the item's name */
    virtual const std::string& GetName(const SharedLock& thisLock) const final { return mName; }

    /** Get the created time stamp */
    virtual Date GetCreated(const SharedLock& thisLock) const final { return mCreated; }

    /** Get the modified time stamp */
    virtual Date GetModified(const SharedLock& thisLock) const final { return mModified; }

    /** Get the accessed time stamp */
    virtual Date GetAccessed(const SharedLock& thisLock) const final { return mAccessed; }

    /** Returns true if the item is read-only */
    virtual bool isReadOnlyFS() const;

    /** 
     * Refresh the item given updated server JSON data
     * @throws BackendImpl::JSONErrorException on JSON errors
     */
    virtual void Refresh(const nlohmann::json& data, const SharedLockW& thisLock);

    /** 
     * Deletes this item (and its contents if a folder) - MUST NOT have an existing parent!
     * @throws BackendException for backend issues
     * @throws HasParentException if it has a parent
    */
    //virtual void DeleteSelf(DeleteLock& deleteLock, const SharedLockW& thisLock); // TODO untested, maybe not needed?

    /** 
     * Delete this item (and its contents if a folder) - MUST have a existing parent!
     * The delete will be done by unlocking self then calling parent->DeleteItem()
     * @param scopeLock reference to scopeLock which will be unlocked
     * @param thisLock temporary lock for self which will be unlocked
     * @throws Folder::NotFoundException if the item is concurrently changed after unlock
     * @throws ReadOnlyFSException if read-only item/filesystem
     * @throws NullParentException if parent is not set
     * @throws BackendException for backend issues
     */
    virtual void Delete(ScopeLocked& scopeLock, SharedLockW& thisLock);

    /** 
     * Set this item's name to the given name, optionally overwrite existing - MUST NOT have an existing parent!
     * @throws BackendException for backend issues
     * @throws HasParentException if it has a parent
     */
    //virtual void RenameSelf(const std::string& newName, const SharedLockW& thisLock, bool overwrite = false); // TODO untested, maybe not needed?

    /** 
     * Set this item's name to the given name, optionally overwrite existing - MUST have a existing parent!
     * The rename will be done by unlocking self then calling parent->RenameItem() then re-locking
     * @param thisLock temporary lock for self which will be temporarily unlocked
     * @throws Folder::NotFoundException if the item is concurrently changed after unlock
     * @throws Folder::DuplicateItemException if name already exists in newParent
     * @throws ReadOnlyFSException if read-only item/filesystem
     * @throws NullParentException if parent is not set
     * @throws InvalidNameException if the name is invalid
     * @throws BackendException for backend issues
     */
    virtual void Rename(const std::string& newName, SharedLockW& thisLock, bool overwrite = false);

    // TODO implement MoveSelf... move is hard because the parent wants the item as a unique_ptr which we can't provide from self
    // maybe have two separate maps - one for objects we own (unique_ptr) and one just for things we point to?

    /** 
     * Move this item to the given parent folder, optionally overwrite existing - MUST have a existing parent!
     * The move will be done by unlocking self then calling parent->MoveItem() then re-locking
     * This will also temporarily get a W lock on the newParent (deadlock-safe)
     * @param thisLock temporary lock for self which will be temporarily unlocked
     * @throws Folder::NotFoundException if the item is concurrently changed after unlock
     * @throws Folder::DuplicateItemException if name already exists in newParent
     * @throws Folder::ModifyException if newParent is a virtual folder
     * @throws ReadOnlyFSException if read-only item/filesystem
     * @throws NullParentException if parent is not set
     * @throws BackendException for backend issues (e.g. if newParent is a subitem of this) // TODO separate exception
     */
    virtual void Move(Folder& newParent, SharedLockW& thisLock, bool overwrite = false);

    /** 
     * Flushes all dirty pages and metadata to the backend
     * NOTE this does NOT happen automatically at destruct!
     * @param nothrow if true, no exceptions are thrown
     * @throws BackendException for backend issues if not nothrow
     */
    virtual void FlushCache(const SharedLockW& thisLock, bool nothrow = false) = 0;

protected:

    /** 
     * Construct a new item
     * @param backend reference to backend
     */
    explicit Item(Backend::BackendImpl& backend);

    /** 
     * Initialize from the given JSON data
     * @throws BackendImpl::JSONErrorException on JSON errors
     */
    Item(Backend::BackendImpl& backend, const nlohmann::json& data);

    friend class Folder; // calls SubDelete(), SubRename(), SubMove(), GetDeleteLock()

    /** Returns the Andromeda object ID */
    virtual const std::string& GetID() { return mId; }

    /**
     * Validates the item's name (no / and not . or ..)
     * @throws InvalidNameException if the name is invalid and not backend
     * @throws BackendImpl::JSONErrorException if the name is invalid and backend
     */
    static void ValidateName(const std::string& name, bool backend = false);

    /** 
     * Item type-specific delete
     * @throws ReadOnlyFSException if read only
     * @throws BackendException for backend issues
     */
    virtual void SubDelete(const DeleteLock& deleteLock) = 0;

    /** 
     * Item type-specific rename
     * @throws ReadOnlyFSException if read only
     * @throws BackendException for backend issues
     */
    virtual void SubRename(const std::string& newName, const SharedLockW& thisLock, bool overwrite) = 0;

    /** 
     * Item type-specific move
     * @throws ReadOnlyFSException if read only
     * @throws BackendException for backend issues
     */
    virtual void SubMove(const std::string& parentID, const SharedLockW& thisLock, bool overwrite) = 0;

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
    mutable std::shared_mutex mScopeMutex;
    /** Primary shared mutex that protects this item */
    mutable SharedMutex mItemMutex;

private:

    mutable Debug mDebug;
};

} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_ITEM_H_
