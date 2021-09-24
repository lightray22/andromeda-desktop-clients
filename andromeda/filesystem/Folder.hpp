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

/** A regular Andromeda folder */
class Folder : public Item
{
public:

    friend class File;

	virtual ~Folder(){};

    /** Exception indicating the item found is not a file */
    class NotFileException : public Utilities::Exception { public:
        NotFileException() : Utilities::Exception("Item is not a File") {}; };

    /** Exception indicating the item found is not a folder */
    class NotFolderException : public Utilities::Exception { public:
        NotFolderException() : Utilities::Exception("Item is not a Folder") {}; };

    /** Exception indicating the requested item already exists */
    class DuplicateItemException : public Utilities::Exception { public:
        DuplicateItemException() : Utilities::Exception("Item already exists") {}; };

    /** 
     * Construct a folder with JSON data
     * @param backend reference to backend 
     * @param parent pointer to parent
     * @param data json data from backend
     * @param haveItems true if JSON has subitems
     */
    Folder(Backend& backend, Folder& parent, const nlohmann::json& data, bool haveItems = false);

    virtual const Type GetType() const override { return Type::FOLDER; }

    /** Load the item with the given relative path */
    Item& GetItemByPath(std::string path);

    /** Load the file with the given relative path */
    File& GetFileByPath(const std::string& path);

    /** Load the folder with the given relative path */
    Folder& GetFolderByPath(const std::string& path);

    typedef std::map<std::string, std::unique_ptr<Item>> ItemMap;

    /** Load the map of child items */
    const ItemMap& GetItems();

    /** Create a new file with the given name */
    virtual void CreateFile(const std::string& name);

    /** Create a new folder with the given name */
    virtual void CreateFolder(const std::string& name);

    virtual void Delete() override;

protected:

    Folder(Backend& backend);

    /** 
     * Construct a folder with JSON data
     * @param backend reference to backend
     * @param data json data from backend
     * @param haveItems true if JSON has subitems
     */
    Folder(Backend& backend, const nlohmann::json& data, bool haveItems = false);

    /** populate itemMap from the backend */
    virtual void LoadItems();

    /** Deletes the item with the given name */
    void RemoveItem(const std::string& name);

private:

    /** Populate itemMap using the given JSON */
    void LoadItems(const nlohmann::json& data);

    /** true if itemMap is loaded */
    bool haveItems;

    /** map of subitems */
    ItemMap itemMap;

    Debug debug;
};

#endif
