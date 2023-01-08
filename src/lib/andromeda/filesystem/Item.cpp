
#include <nlohmann/json.hpp>

#include "Item.hpp"
#include "Folder.hpp"
#include "FSConfig.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;

namespace Andromeda {
namespace Filesystem {

/*****************************************************/
Item::Item(BackendImpl& backend) : 
    mBackend(backend), mDebug("Item",this)
{
    MDBG_INFO("()");
}

/*****************************************************/
void Item::Initialize(const nlohmann::json& data)
{
    MDBG_INFO("()");

    try
    {
        data.at("id").get_to(mId);
        data.at("name").get_to(mName);

        if (data.contains("dates"))
        {
            data.at("dates").at("created").get_to(mCreated);
            
            const nlohmann::json& modifiedJ(data.at("dates").at("modified"));
            if (!modifiedJ.is_null()) modifiedJ.get_to(mModified);

            const nlohmann::json& accessedJ(data.at("dates").at("accessed"));
            if (!accessedJ.is_null()) accessedJ.get_to(mAccessed);
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

/*****************************************************/
void Item::Refresh(const nlohmann::json& data)
{
    MDBG_INFO("(" << mName << ")" << " ()");

    try
    {
        decltype(mName) newName; 
        data.at("name").get_to(newName);
        if (newName != mName)
        {
            mName = newName;
            MDBG_INFO("... newName:" << mName);
        }

        const nlohmann::json& modifiedJ(data.at("dates").at("modified"));
        if (!modifiedJ.is_null())
        {
            decltype(mModified) newModified; 
            modifiedJ.get_to(newModified);
            if (newModified != mModified)
            {
                mModified = newModified;
                MDBG_INFO("... newModified:" << mModified);
            }
        }

        const nlohmann::json& accessedJ(data.at("dates").at("accessed"));
        if (!accessedJ.is_null())
        {
            decltype(mAccessed) newAccessed; 
            accessedJ.get_to(newAccessed);
            if (newAccessed != mAccessed)
            {
                mAccessed = newAccessed;
                MDBG_INFO("... newAccessed:" << mAccessed);
            }
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

/*****************************************************/
Folder& Item::GetParent() const     
{
    if (mParent == nullptr) 
        throw NullParentException();
            
    return *mParent; 
}

/*****************************************************/
const FSConfig& Item::GetFSConfig() const
{
    if (mFsConfig == nullptr)
        throw NullFSConfigException();

    return *mFsConfig;
}

/*****************************************************/
bool Item::isReadOnly() const
{
    bool retval { mBackend.isReadOnly() };

    if (HasFSConfig()) retval |= GetFSConfig().isReadOnly();

    return retval;
}

/*****************************************************/
void Item::Delete(bool internal)
{
    if (internal || !HasParent()) SubDelete();
    else GetParent().DeleteItem(mName);
}

/*****************************************************/
void Item::Rename(const std::string& newName, bool overwrite, bool internal)
{
    if (internal || !HasParent())
    {
        SubRename(newName, overwrite); 
        mName = newName;
    }
    else GetParent().RenameItem(mName, newName, overwrite);
}

/*****************************************************/
void Item::Move(Folder& newParent, bool overwrite, bool internal)
{
    if (internal)
    {
        SubMove(newParent, overwrite); 
        mParent = &newParent;
    }
    else GetParent().MoveItem(mName, newParent, overwrite);
}

} // namespace Filesystem
} // namespace Andromeda
