#ifndef LIBA2_ITEM_H_
#define LIBA2_ITEM_H_

#include <string>

#include <nlohmann/json_fwd.hpp>

#include "andromeda/BaseException.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda {

namespace Backend { class BackendImpl; }

namespace Filesystem {
class FSConfig;

class Folder;

/** An abstract item in a filesystem */
class Item
{
public:

    class Exception : public BaseException { public:
        using BaseException::BaseException; };
    
    /** Exception indicating this item has no parent */
    class NullParentException : public Exception { public:
        NullParentException() : Exception("Null Parent") {}; };

    /** Exception indicating the item has no filesystem */
    class NullFSConfigException : public Exception { public:
        NullFSConfigException() : Exception("Null FSConfig") {}; };

    /** Exception indicating the item is read-only */
    class ReadOnlyException : public Exception { public:
        ReadOnlyException() : Exception("Read Only Backend") {}; };

    /** Macro to print the file name at the beginning of debug */
    #define ITDBG_INFO(strfunc) MDBG_INFO("(" << mName << ")" << strfunc)

    virtual ~Item(){};

    /** API date format */
    typedef double Date;

    /** Concrete item types */
    enum class Type { FILE, FOLDER };

    /** Returns the FS type */
    virtual Type GetType() const = 0;

    /** Returns the Andromeda object ID */
    virtual const std::string& GetID() { return mId; }

    /** Returns the FS name */
    virtual const std::string& GetName() const final { return mName; }

    /** Returns a reference to the backend for this item */
    virtual Backend::BackendImpl& GetBackend() const final { return mBackend; }

    /** Returns true if this item has a parent */
    virtual bool HasParent() const { return mParent != nullptr; }

    /** Returns the parent folder */
    virtual Folder& GetParent() const;

    /** Returns true if this item has FSconfig */
    virtual bool HasFSConfig() const { return mFsConfig != nullptr; }

    /** Returns the filesystem config */
    virtual const FSConfig& GetFSConfig() const;

    /** Get the created time stamp */
    virtual Date GetCreated() const final { return mCreated; } 

    /** Get the modified time stamp */
    virtual Date GetModified() const final { return mModified; } 

    /** Get the accessed time stamp */
    virtual Date GetAccessed() const final { return mAccessed; } 

    /** Returns true if the item is read-only */
    virtual bool isReadOnly() const;

    /** Refresh the item given updated server JSON data */
    virtual void Refresh(const nlohmann::json& data);

    /** Delete this item (and its contents if a folder) */
    virtual void Delete(bool internal = false) final; // TODO refactor to get rid of "internal"

    /** Set this item's name to the given name, optionally overwrite existing */
    virtual void Rename(const std::string& newName, bool overwrite = false, bool internal = false) final;

    /** Move this item to the given parent folder, optionally overwrite existing */
    virtual void Move(Folder& newParent, bool overwrite = false, bool internal = false) final;

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
    Item(Backend::BackendImpl& backend);

    /** Initialize from the given JSON data */
    virtual void Initialize(const nlohmann::json& data);

    /** Item type-specific delete */
    virtual void SubDelete() = 0;

    /** Item type-specific rename */
    virtual void SubRename(const std::string& newName, bool overwrite) = 0;

    /** Item type-specific move */
    virtual void SubMove(Folder& newParent, bool overwrite) = 0;
    
    /** Reference to the API backend */
    Backend::BackendImpl& mBackend;

    /** Pointer to parent folder */
    Folder* mParent { nullptr };

    /** Pointer to filesystem config */
    const FSConfig* mFsConfig { nullptr };

    /** Backend object ID */
    std::string mId;

    /** Name of the item */
    std::string mName;

    /** Item creation timestamp */
    Date mCreated { 0 };

    /** Item modified timestamp */
    Date mModified { 0 };

    /** Item accessed timestamp */
    Date mAccessed { 0 };

private:

    Debug mDebug;

    Item(const Item&) = delete; // no copy
};

} // namespace Filesystem
} // namespace Andromeda

#endif // LIBA2_ITEM_H_
