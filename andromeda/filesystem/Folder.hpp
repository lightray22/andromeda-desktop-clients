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

    /** Base Exception for all backend issues */
    class NotFileException : public Utilities::Exception { public:
        NotFileException() : Utilities::Exception("Item is not a File") {}; };

    /** Base Exception for all backend issues */
    class NotFolderException : public Utilities::Exception { public:
        NotFolderException() : Utilities::Exception("Item is not a Folder") {}; };

    /**
     * Load a folder from the backend with the given ID
     * @param backend reference to backend
     * @param id ID of folder to load
     */
    static std::unique_ptr<Folder> LoadByID(Backend& backend, const std::string& id);

    /** 
     * @param backend reference to backend 
     * @param data json data from backend
     * @param haveItems true if JSON has subitems
     */
    Folder(Backend& backend, const nlohmann::json& data, bool haveItems);

    virtual const Type GetType() const override { return Type::FOLDER; }

    /** Load the item with the given relative path */
    Item& GetItemByPath(const std::string& path);

    /** Load the file with the given relative path */
    File& GetFileByPath(const std::string& path);

    /** Load the folder with the given relative path */
    Folder& GetFolderByPath(const std::string& path);

    typedef std::map<std::string, std::unique_ptr<Item>> ItemMap;

    /** Load the map of child items */
    const ItemMap& GetItems();

protected:

    Folder(Backend& backend);

    /** populate itemMap from the backend */
    virtual void LoadItems();

    /** true if itemMap is loaded */
    bool haveItems;

    /** map of subitems */
    ItemMap itemMap;

private:

    /** Populate itemMap using the given JSON */
    void LoadItems(const nlohmann::json& data);

    Debug debug;
};

#endif