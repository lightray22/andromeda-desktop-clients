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
    
    /** Exception indicating this folder has no parent */
    class NullParentException : public Exception { public:
        NullParentException() : Exception("Folder parent is null") {}; };

    virtual const Type GetType() const override final { return Type::FOLDER; }

    /** Returns true if this item has a parent */
    virtual const bool HasParent() const { return this->parent != nullptr; }

    /** Returns the parent folder */
    virtual Folder& GetParent() const;

    /** Load the item with the given relative path */
    virtual Item& GetItemByPath(std::string path) final;

    /** Load the file with the given relative path */
    virtual File& GetFileByPath(const std::string& path) final;

    /** Load the folder with the given relative path */
    virtual Folder& GetFolderByPath(const std::string& path) final;

    typedef std::map<std::string, std::unique_ptr<Item>> ItemMap;

    /** Load the map of child items */
    virtual const ItemMap& GetItems() final;

    /** Create a new file with the given name */
    virtual void CreateFile(const std::string& name);

    /** Create a new folder with the given name */
    virtual void CreateFolder(const std::string& name);

    /** Remove the item with the given name */
    virtual void RemoveItem(const std::string& name) final;

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

    virtual void SubCreateFile(const std::string& name) = 0;

    virtual void SubCreateFolder(const std::string& name) = 0;

    virtual void SubRemoveItem(Item& item) = 0;

    /** map of subitems */
    ItemMap itemMap;

    Folder* parent;

private:

    /** true if itemMap is loaded */
    bool haveItems;

    Debug debug;
};

#endif
