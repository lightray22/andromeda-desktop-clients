#ifndef LIBA2_FOLDER_H_
#define LIBA2_FOLDER_H_

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <nlohmann/json_fwd.hpp>

#include "Item.hpp"
#include "File.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/ScopeLocked.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {

/** A common folder interface */
class Folder : public Item
{
public:

    ~Folder() override = default;

    /** Base Exception for all folder issues */
    class Exception : public Item::Exception { public:
        /** @param message Folder error string */
        explicit Exception(const std::string& message) :
            Item::Exception("Folder Error: "+message) {}; };

    /** Exception indicating the item found is not a file */
    class NotFileException : public Exception { public:
        NotFileException() : Exception("Not a File") {}; };

    /** Exception indicating the item found is not a folder */
    class NotFolderException : public Exception { public:
        NotFolderException() : Exception("Not a Folder") {}; };

    /** Exception indicating the item was not found */
    class NotFoundException : public Exception { public:
        NotFoundException() : Exception("Not Found") {}; };

    /** Exception indicating the requested item already exists */
    class DuplicateItemException : public Exception { public:
        DuplicateItemException() : Exception("Already Exists") {}; };

    /** Exception indicating the item cannot be modified */
    class ModifyException : public Exception { public:
        ModifyException() : Exception("Immutable Item") {}; };

    using ScopeLocked = Andromeda::ScopeLocked<Folder>;
    /** Tries to lock mScopeMutex, returns a ref that is maybe locked */
    inline ScopeLocked TryLockScope() { return ScopeLocked(*this, mScopeMutex); }

    Type GetType() const final { return Type::FOLDER; }

    /** 
     * Load the item with the given relative path, returning it with a pre-checked ScopeLock 
     * Will get a write lock on this folder - DO NOT ACQUIRE FIRST!
     */
    virtual Item::ScopeLocked GetItemByPath(std::string path) final;

    /** Load the file with the given relative path, returning it with a pre-checked ScopeLock */
    virtual File::ScopeLocked GetFileByPath(const std::string& path) final;

    /** Load the folder with the given relative path, returning it with a pre-checked ScopeLock */
    virtual Folder::ScopeLocked GetFolderByPath(const std::string& path) final;

    /** Map of sub-item name to ScopeLocked Item objects */
    using LockedItemMap = std::map<std::string, Item::ScopeLocked>;

    /** 
     * Load and return the map of scope-locked child items 
     * @param thisLock need an exclusive lock since we may reload the map
     * @return map of all children with accompanying scope locks
     */
    virtual LockedItemMap GetItems(const SharedLockW& thisLock) final;

    /** Returns the count of child items */
    virtual size_t CountItems(const SharedLockW& thisLock) final;

    /** Create a new subfile with the given name */
    virtual void CreateFile(const std::string& name, const SharedLockW& thisLock) final;

    /** Create a new subfolder with the given name */
    virtual void CreateFolder(const std::string& name, const SharedLockW& thisLock) final;

    void FlushCache(const Andromeda::SharedLockW& thisLock, bool nothrow = false) override;

protected:

    /** 
     * Construct a new abstract folder
     * @param backend reference to backend
     */
    explicit Folder(Backend::BackendImpl& backend);

    /** Initialize from the given JSON data */
    Folder(Backend::BackendImpl& backend, const nlohmann::json& data);

    friend class Item; // calls DeleteItem(), RenameItem(), MoveItem()

    /** Delete the subitem with the given name */
    virtual void DeleteItem(const std::string& name, const SharedLockW& thisLock) final;

    /** Rename the subitem oldName to newName, optionally overwrite */
    virtual void RenameItem(const std::string& oldName, const std::string& newName, 
        const SharedLockW& thisLock, bool overwrite = false) final;

    /** 
     * Move the subitem name to parent folder, optionally overwrite
     * @param itemsLocks a lock pair for both this and the new parent
     */
    virtual void MoveItem(const std::string& name, Folder& newParent, 
        const SharedLockW::LockPair& itemLocks, bool overwrite = false) final;

    using UniqueLock = std::unique_lock<std::mutex>;

    /** Makes sure mItemMap is populated and refreshed */
    virtual void LoadItems(const SharedLockW& thisLock, bool force = false);

    /** Map consisting of an item name -> write lock for the item */
    using ItemLockMap = std::map<std::string, SharedLockW>;

    /** Populates the item list with items from the backend */
    virtual void SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& thisLock) = 0;

    /** Function that returns a new Item given its JSON data */
    using NewItemFunc = std::function<std::unique_ptr<Item> (const nlohmann::json&)>;

    /** Map consisting of an item name -> a pair of its JSON data and construct function */
    using NewItemMap = std::map<std::string, std::pair<const nlohmann::json&, NewItemFunc>>;

    /** 
     * Synchronizes in-memory content using the given new item map
     * @param newItems map with items JSON from the backend
     * @param itemsLocks read locks for every item, locked before the backend was read
     * @param thisLock writeLock for this folder
     */
    virtual void SyncContents(const NewItemMap& newItems, ItemLockMap& itemsLocks, const SharedLockW& thisLock);

    /** The folder-type-specific create subfile */
    virtual void SubCreateFile(const std::string& name, const SharedLockW& thisLock) = 0;

    /** The folder-type-specific create subfolder */
    virtual void SubCreateFolder(const std::string& name, const SharedLockW& thisLock) = 0;

    /** Map of sub-item name to Item objects */
    using ItemMap = std::map<std::string, std::unique_ptr<Item>>;

    /** map of subitems */
    ItemMap mItemMap;

    /** true if itemMap is loaded */
    bool mHaveItems { false };

    /** time point when contents were loaded */
    std::chrono::steady_clock::time_point mRefreshed;

private:

    mutable Debug mDebug;
};

} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_FOLDER_H_
