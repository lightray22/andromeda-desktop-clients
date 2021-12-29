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

    /** API date format */
    typedef double Date;

    /** Concrete item types */
    enum class Type { FILE, FOLDER };

    /** Returns the FS type */
    virtual Type GetType() const = 0;

    /** Returns the Andromeda object ID */
    virtual const std::string& GetID() { return this->id; }

    /** Returns the FS name */
    virtual const std::string& GetName() const final { return this->name; }

    /** Returns the total size */
    virtual size_t GetSize() const final { return this->size; }

    /** Get the created time stamp */
    virtual Date GetCreated() const final { return this->created; } 

    /** Get the modified time stamp */
    virtual Date GetModified() const final { return this->modified; } 

    /** Get the accessed time stamp */
    virtual Date GetAccessed() const final { return this->accessed; } 

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

    /** 
     * Flushes all dirty pages to the backend 
     * @param nothrow if true, no exceptions are thrown
     */
    virtual void FlushCache(bool nothrow = false) = 0;

protected:

    /** 
     * Construct a new item
     * @param backend reference to backend
     */
    Item(Backend& backend);

    /** Initialize from the given JSON data */
    virtual void Initialize(const nlohmann::json& data);

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
    
    /** Reference to the API backend */
    Backend& backend;

    /** Pointer to parent folder */
    Folder* parent = nullptr;

    /** Pointer to filesystem config */
    const FSConfig* fsConfig = nullptr;

    /** Backend object ID */
    std::string id;

    /** Name of the item */
    std::string name;

    /** Size of the item in bytes */
    size_t size = 0;

    /** Item creation timestamp */
    Date created = 0;

    /** Item modified timestamp */
    Date modified = 0;

    /** Item accessed timestamp */
    Date accessed = 0;

private:

    Debug debug;
};

#endif
