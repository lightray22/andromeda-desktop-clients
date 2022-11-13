#ifndef LIBA2_FOLDER_H_
#define LIBA2_FOLDER_H_

#include <string>
#include <map>
#include <memory>
#include <chrono>
#include <functional>
#include <nlohmann/json_fwd.hpp>

#include "Item.hpp"
#include "andromeda/Utilities.hpp"

#if WIN32 && defined(CreateFile)
// thanks for nothing, Windows >:(
#undef CreateFile
#endif

namespace Andromeda {
class Backend;

namespace FSItems {
class File;

/** A common folder interface */
class Folder : public Item
{
public:

    virtual ~Folder(){};

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

    virtual Type GetType() const override final { return Type::FOLDER; }

    /** Load the item with the given relative path */
    virtual Item& GetItemByPath(std::string path) final;

    /** Load the file with the given relative path */
    virtual File& GetFileByPath(const std::string& path) final;

    /** Load the folder with the given relative path */
    virtual Folder& GetFolderByPath(const std::string& path) final;

    /** Map of sub-item name to Item objects */
    typedef std::map<std::string, std::unique_ptr<Item>> ItemMap;

    /** Load the map of child items */
    virtual const ItemMap& GetItems() final;

    /** Create a new subfile with the given name */
    virtual void CreateFile(const std::string& name) final;

    /** Create a new subfolder with the given name */
    virtual void CreateFolder(const std::string& name) final;

    /** Delete the subitem with the given name */
    virtual void DeleteItem(const std::string& name) final;

    /** Rename the subitem oldName to newName, optionally overwrite */
    virtual void RenameItem(const std::string& oldName, const std::string& newName, bool overwrite = false) final;

    /** Move the subitem name to parent folder, optionally overwrite */
    virtual void MoveItem(const std::string& name, Folder& newParent, bool overwrite = false) final;

    virtual void FlushCache(bool nothrow = false) override;

protected:

    /** 
     * Construct a new abstract folder
     * @param backend reference to backend
     */
    Folder(Backend& backend);

    /** populate itemMap from the backend */
    virtual void LoadItems() = 0;

    /** Populate/merge itemMap using the given JSON */
    virtual void LoadItemsFrom(const nlohmann::json& data);

    /** Function that returns a new Item given its JSON data */
    typedef std::function<std::unique_ptr<Item>(const nlohmann::json&)> NewItemFunc;

    /** Map consisting of an item name -> a pair of its JSON data and construct function */
    typedef std::map<std::string, std::pair<const nlohmann::json&, NewItemFunc>> NewItemMap;

    /** Synchronizes in-memory content using the given map with JSON from the backend */
    virtual void SyncContents(const NewItemMap& newItems);

    /** The folder-type-specific create subfile */
    virtual void SubCreateFile(const std::string& name) = 0;

    /** The folder-type-specific create subfolder */
    virtual void SubCreateFolder(const std::string& name) = 0;

    /** The folder-type-specific delete subitem */
    virtual void SubDeleteItem(Item& item) = 0;

    /** The folder-type-specific rename subitem */
    virtual void SubRenameItem(Item& item, const std::string& newName, bool overwrite) = 0;

    /** The folder-type-specific move subitem */
    virtual void SubMoveItem(Item& item, Folder& newParent, bool overwrite) = 0;

    /** map of subitems */
    ItemMap mItemMap;

    /** Returns true iff the itemMap is loaded */
    virtual bool HaveItems() const { return mHaveItems; }

private:

    /** true if itemMap is loaded */
    bool mHaveItems { false };

    /** time point when contents were loaded */
    std::chrono::steady_clock::time_point mRefreshed;

    Debug mDebug;
};

} // namespace FSItems
} // namespace Andromeda

#endif
