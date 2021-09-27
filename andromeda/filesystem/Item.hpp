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

    class Exception : public Utilities::Exception { public:
        using Utilities::Exception::Exception; };

	virtual ~Item(){};

    typedef unsigned long long Size;
    typedef double Date;

    enum class Type { FILE, FOLDER };

    /** Returns the FS type */
    virtual const Type GetType() const = 0;

    /** Returns true if this item has a parent */
    virtual const bool HasParent() const = 0;

    /** Returns the parent folder */
    virtual Folder& GetParent() const = 0;

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

    /** Remove this item from its parent */
    virtual void Delete(bool internal = false);

    /** Set this item's name to the given name, optionally overwrite */
    virtual void Rename(const std::string& name, bool overwrite = false, bool internal = false);

protected:

    /** 
     * Construct without initializing
     * @param backend reference to backend
     */
    Item(Backend& backend);

    /** 
     * Construct with JSON data
     * @param backend reference to backend
     * @param data json data from backend
     */
    Item(Backend& backend, const nlohmann::json& data);

    /** Item type-specific delete */
    virtual void SubDelete() = 0;

    /** Item type-specific rename */
    virtual void SubRename(const std::string& name, bool overwrite) = 0;
    
    Backend& backend;

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
