#ifndef LIBA2_ITEM_H_
#define LIBA2_ITEM_H_

#include <string>

#include <nlohmann/json_fwd.hpp>

#include "Utilities.hpp"

class Backend;

/** An abstract item in a filesystem */
class Item
{
public:

    typedef unsigned long long Size;
    typedef double Date;

    enum class Type { FILE, FOLDER };

    /** Returns the FS type */
    virtual const Type GetType() const = 0;

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

protected:

    /** @param backend reference to backend */
    Item(Backend& backend);

    /** @param data data loaded from the backend */
    virtual void Initialize(const nlohmann::json& data);
    
    Backend& backend;

    std::string name;

    Size size = 0;

    Date created = 0;
    Date modified = 0;
    Date accessed = 0;

private:

    Debug debug;
};

#endif