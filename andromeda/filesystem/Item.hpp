#ifndef LIBA2_ITEM_H_
#define LIBA2_ITEM_H_

#include <string>

#include <nlohmann/json_fwd.hpp>

#include "Utilities.hpp"

class Backend;
class Folder;

/** An abstract item in a filesystem */
class Item
{
public:

    /** Exception indicating this item has no parent */
    class NullParentException : public Utilities::Exception { public:
        NullParentException() : Utilities::Exception("Item parent is null") {}; };

	virtual ~Item(){};

    typedef unsigned long long Size;
    typedef double Date;

    enum class Type { FILE, FOLDER };

    /** Returns the FS type */
    virtual const Type GetType() const = 0;

    /** Returns true if this item has a parent */
    virtual const bool HasParent() const { return this->parent != nullptr; }

    /** Returns the parent folder */
    virtual Folder& GetParent() const;

    /** Returns the Andromeda object ID */
    virtual const std::string& GetID() const { return this->id; }

    /** Returns the FS name */
    virtual const std::string& GetName() const { return this->name; }

    /** Returns the total size */
    virtual const Size GetSize() const { return this->size; }

    /** Get the created time stamp */
    virtual const Date GetCreated() const { return this->created; } 

    /** Get the modified time stamp */
    virtual const Date GetModified() const { return this->modified; } 

    /** Get the accessed time stamp */
    virtual const Date GetAccessed() const { return this->accessed; } 

    /** Delete this item */
    virtual void Delete() = 0;

protected:

    /** 
     * Construct an item w/o initializing
     * @param backend reference to backend
     */
    Item(Backend& backend);

    /** 
     * Construct an item with JSON data
     * @param backend reference to backend
     * @param data json data from backend
     */
    Item(Backend& backend, const nlohmann::json& data);

    /** 
     * Construct an item with JSON data
     * @param backend reference to backend 
     * @param parent pointer to parent
     * @param data json data from backend
     */
    Item(Backend& backend, Folder& parent, const nlohmann::json& data);
    
    Backend& backend;

    Folder* parent;

    std::string id;
    std::string name;

    Size size = 0;

    Date created = 0;
    Date modified = 0;
    Date accessed = 0;

private:

    Debug debug;
};

#endif
