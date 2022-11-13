
#include <nlohmann/json.hpp>

#include "Item.hpp"
#include "Folder.hpp"
#include "andromeda/Backend.hpp"
#include "andromeda/FSConfig.hpp"

namespace Andromeda {
namespace FSItems {

/*****************************************************/
Item::Item(Backend& backend) : 
    mBackend(backend), mDebug("Item",this)
{
    mDebug << __func__ << "()"; mDebug.Info();
}

/*****************************************************/
void Item::Initialize(const nlohmann::json& data)
{
    mDebug << __func__ << "()"; mDebug.Info();

    try
    {
        data.at("id").get_to(mId);
        data.at("name").get_to(mName);

        if (data.contains("dates"))
        {
            data.at("dates").at("created").get_to(mCreated);
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }

    Refresh(data);
}

/*****************************************************/
void Item::Refresh(const nlohmann::json& data)
{
    mDebug << __func__ << "()"; mDebug.Info();

    try
    {
        data.at("name").get_to(mName);

        mDebug << __func__ << "... name:" << mName; mDebug.Info();

        const nlohmann::json& modifiedJ(data.at("dates").at("modified"));
        if (!modifiedJ.is_null()) modifiedJ.get_to(mModified);

        const nlohmann::json& accessedJ(data.at("dates").at("accessed"));
        if (!accessedJ.is_null()) accessedJ.get_to(mAccessed);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
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
    const Config& config(mBackend.GetConfig());

    bool retval { config.isReadOnly() || config.GetOptions().readOnly };

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

} // namespace FSItems
} // namespace Andromeda
