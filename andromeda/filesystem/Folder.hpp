#ifndef LIBA2_FOLDER_H_
#define LIBA2_FOLDER_H_

#include <string>
#include <map>
#include <memory>
#include <nlohmann/json_fwd.hpp>

#include "Item.hpp"
#include "Utilities.hpp"

class Backend;
class File;

/** A common folder interface */
class Folder : public Item
{
public:

    virtual ~Folder(){};

    /** Exception indicating the item found is not a file */
    class NotFileException : public Exception { public:
        NotFileException() : Exception("Item is not a File") {}; };

    /** Exception indicating the item found is not a folder */
    class NotFolderException : public Exception { public:
        NotFolderException() : Exception("Item is not a Folder") {}; };

    /** Exception indicating the requested item already exists */
    class DuplicateItemException : public Exception { public:
        DuplicateItemException() : Exception("Item already exists") {}; };

    class ModifyException : public Exception { public:
        ModifyException() : Exception("Item cannot be modified") {}; };

    virtual Type GetType() const override final { return Type::FOLDER; }

    /** Load the item with the given relative path */
    virtual Item& GetItemByPath(std::string path) final;

    /** Load the file with the given relative path */
    virtual File& GetFileByPath(const std::string& path) final;

    /** Load the folder with the given relative path */
    virtual Folder& GetFolderByPath(const std::string& path) final;

    typedef std::map<std::string, std::unique_ptr<Item>> ItemMap;

    /** Load the map of child items */
    virtual const ItemMap& GetItems() final;

    /** Create a new subfile with the given name */
    virtual void CreateFile(const std::string& name) final;

    /** Create a new subfolder with the given name */
    virtual void CreateFolder(const std::string& name) final;

    /** Delete the subitem with the given name */
    virtual void DeleteItem(const std::string& name) final;

    /** Rename the subitem name0 to name1, optionally overwrite */
    virtual void RenameItem(const std::string& name0, const std::string& name1, bool overwrite = false) final;

    /** Move the subitem name to parent folder, optionally overwrite */
    virtual void MoveItem(const std::string& name, Folder& parent, bool overwrite = false) final;

protected:

    /** 
     * Construct without initializing
     * @param backend reference to backend
     */
    Folder(Backend& backend);

    /** 
     * Construct with JSON data
     * @param backend reference to backend
     * @param data json data from backend
     */
    Folder(Backend& backend, const nlohmann::json& data);

    /** populate itemMap from the backend */
    virtual void LoadItems() = 0;

    /** Populate itemMap using the given JSON */
    virtual void LoadItemsFrom(const nlohmann::json& data);

    /** The folder-type-specific create subfile */
    virtual void SubCreateFile(const std::string& name) = 0;

    /** The folder-type-specific create subfolder */
    virtual void SubCreateFolder(const std::string& name) = 0;

    /** The folder-type-specific delete subitem */
    virtual void SubDeleteItem(Item& item) = 0;

    /** The folder-type-specific rename subitem */
    virtual void SubRenameItem(Item& item, const std::string& name, bool overwrite) = 0;

    /** The folder-type-specific move subitem */
    virtual void SubMoveItem(Item& item, Folder& parent, bool overwrite) = 0;

    /** Whether or not this folder can have items moved into it */
    virtual bool CanReceiveItems() = 0;

    /** map of subitems */
    ItemMap itemMap;

private:

    /** true if itemMap is loaded */
    bool haveItems = false;

    Debug debug;
};

#endif
