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

    virtual ~Folder(){ };

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

    typedef Andromeda::ScopeLocked<Folder> ScopeLocked;
    /** Tries to lock mScopeMutex, returns a ref that is maybe locked */
    inline ScopeLocked TryLockScope() { return ScopeLocked(*this, mScopeMutex); }

    virtual Type GetType() const override final { return Type::FOLDER; }

    /** Load the item with the given relative path, returning it with a pre-checked ScopeLock */
    virtual Item::ScopeLocked GetItemByPath(std::string path) final;

    /** Load the file with the given relative path, returning it with a pre-checked ScopeLock */
    virtual File::ScopeLocked GetFileByPath(const std::string& path) final;

    /** Load the folder with the given relative path, returning it with a pre-checked ScopeLock */
    virtual Folder::ScopeLocked GetFolderByPath(const std::string& path) final;

    /** Map of sub-item name to ScopeLocked Item objects */
    typedef std::map<std::string, Item::ScopeLocked> LockedItemMap;

    /** 
     * Load and return the map of scope-locked child items 
     * Will acquire a write lock as this may involve reloading from the server!
     * @return map of all children with accompanying scope locks
     */
    virtual LockedItemMap GetItems() final;
    virtual LockedItemMap GetItems(const SharedLockW& itemLock) final;

    /** Returns the count of child items */
    virtual size_t CountItems(const SharedLockW& itemLock) final;

    /** Create a new subfile with the given name */
    virtual void CreateFile(const std::string& name, const SharedLockW& itemLock) final;

    /** Create a new subfolder with the given name */
    virtual void CreateFolder(const std::string& name, const SharedLockW& itemLock) final;

    /** Delete the subitem with the given name */
    virtual void DeleteItem(const std::string& name, const SharedLockW& itemLock) final;

    /** Rename the subitem oldName to newName, optionally overwrite */
    virtual void RenameItem(const std::string& oldName, const std::string& newName, 
        const SharedLockW& itemLock, bool overwrite = false) final;

    /** 
     * Move the subitem name to parent folder, optionally overwrite 
     * @param itemsLocks a lock pair for both this and the new parent
     */
    virtual void MoveItem(const std::string& name, Folder& newParent, 
        const SharedLockW::LockPair& itemLocks, bool overwrite = false) final;

    virtual void FlushCache(const Andromeda::SharedLockW& itemLock, bool nothrow = false) override;

protected:

    /** 
     * Construct a new abstract folder
     * @param backend reference to backend
     */
    Folder(Backend::BackendImpl& backend);

    /** Initialize from the given JSON data */
    Folder(Backend::BackendImpl& backend, const nlohmann::json& data);

    typedef std::unique_lock<std::mutex> UniqueLock;

    /** Makes sure mItemMap is populated and refreshed */
    virtual void LoadItems(const SharedLockW& itemLock, bool force = false);

    /** Map consisting of an item name -> read lock for the item */
    typedef std::map<std::string, SharedLockR> ItemLockMap;

    /** Populates the item list with items from the backend */
    virtual void SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& itemLock) = 0;

    /** Function that returns a new Item given its JSON data */
    typedef std::function<std::unique_ptr<Item>(const nlohmann::json&)> NewItemFunc;

    /** Map consisting of an item name -> a pair of its JSON data and construct function */
    typedef std::map<std::string, std::pair<const nlohmann::json&, NewItemFunc>> NewItemMap;

    /** 
     * Synchronizes in-memory content using the given new item map
     * @param newItems map with items JSON from the backend
     * @param itemsLocks read locks for every item, locked before the backend was read
     * @param itemLock writeLock for this folder
     */
    virtual void SyncContents(const NewItemMap& newItems, ItemLockMap& itemsLocks, const SharedLockW& itemLock);

    /** The folder-type-specific create subfile */
    virtual void SubCreateFile(const std::string& name, const SharedLockW& itemLock) = 0;

    /** The folder-type-specific create subfolder */
    virtual void SubCreateFolder(const std::string& name, const SharedLockW& itemLock) = 0;

    /** Map of sub-item name to Item objects */
    typedef std::map<std::string, std::unique_ptr<Item>> ItemMap;

    /** map of subitems */
    ItemMap mItemMap;

    /** true if itemMap is loaded */
    bool mHaveItems { false };

    /** time point when contents were loaded */
    std::chrono::steady_clock::time_point mRefreshed;

private:

    Debug mDebug;
};

} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_FOLDER_H_
