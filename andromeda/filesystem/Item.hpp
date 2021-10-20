#ifndef LIBA2_ITEM_H_
#define LIBA2_ITEM_H_

#include <string>

#include <nlohmann/json_fwd.hpp>

#include "Utilities.hpp"

class Backend;
class Folder;
class FSConfig;

/** An abstract item in a filesystem */
class Item
{
public:

    class Exception : public Utilities::Exception { public:
        using Utilities::Exception::Exception; };
    
    /** Exception indicating this item has no parent */
    class NullParentException : public Exception { public:
        NullParentException() : Exception("Null Parent") {}; };

    /** Exception indicating the item has no filesystem */
    class NullFSConfigException : public Exception { public:
        NullFSConfigException() : Exception("Null FSConfig") {}; };

    /** Exception indicating the item is read-only */
    class ReadOnlyException : public Exception { public:
        ReadOnlyException() : Exception("Read Only Backend") {}; };

    virtual ~Item(){};

    typedef double Date;

    enum class Type { FILE, FOLDER };

    /** Returns the FS type */
    virtual Type GetType() const = 0;

    /** Returns the Andromeda object ID */
    virtual const std::string& GetID() const { return this->id; }

    /** Returns the FS name */
    virtual const std::string& GetName() const { return this->name; }

    /** Returns the total size */
    virtual size_t GetSize() const { return this->size; }

    /** Get the created time stamp */
    virtual Date GetCreated() const { return this->created; } 

    /** Get the modified time stamp */
    virtual Date GetModified() const { return this->modified; } 

    /** Get the accessed time stamp */
    virtual Date GetAccessed() const { return this->accessed; } 

    /** Returns true if the item is read-only */
    virtual bool isReadOnly() const;

    /** Refresh the item given updated server JSON data */
    virtual void Refresh(const nlohmann::json& data);

    /** Remove this item from its parent */
    virtual void Delete(bool internal = false) final;

    /** Set this item's name to the given name, optionally overwrite */
    virtual void Rename(const std::string& name, bool overwrite = false, bool internal = false) final;

    /** Move this item to the given parent folder, optionally overwrite */
    virtual void Move(Folder& parent, bool overwrite = false, bool internal = false) final;

    /** Flushes all dirty pages to the backend */
    virtual void FlushCache() = 0;

protected:

    /** 
     * Construct a new item
     * @param backend reference to backend
     * @param data pointer to JSON data
     */
    Item(Backend& backend, const nlohmann::json* data = nullptr);

    /** Returns true if this item has a parent */
    virtual bool HasParent() const { return this->parent != nullptr; }

    /** Returns the parent folder */
    virtual Folder& GetParent() const;

    /** Returns true if this item has FSconfig */
    virtual bool HasFSConfig() const { return this->fsConfig != nullptr; }

    /** Returns the filesystem config */
    virtual const FSConfig& GetFSConfig() const;

    /** Item type-specific delete */
    virtual void SubDelete() = 0;

    /** Item type-specific rename */
    virtual void SubRename(const std::string& name, bool overwrite) = 0;

    /** Item type-specific move */
    virtual void SubMove(Folder& parent, bool overwrite) = 0;
    
    Backend& backend;

    Folder* parent = nullptr;

    const FSConfig* fsConfig = nullptr;

    std::string id;
    std::string name;

    size_t size = 0;

    Date created = 0;
    Date modified = 0;
    Date accessed = 0;

private:

    Debug debug;
};

#endif
