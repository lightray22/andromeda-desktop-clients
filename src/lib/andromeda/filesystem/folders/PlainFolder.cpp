
#include <nlohmann/json.hpp>

#include "PlainFolder.hpp"
#include "andromeda/ConfigOptions.hpp"
using Andromeda::ConfigOptions;
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/FSConfig.hpp"
using Andromeda::Filesystem::FSConfig;
#include "andromeda/filesystem/File.hpp"
using Andromeda::Filesystem::File;

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/*****************************************************/
std::unique_ptr<PlainFolder> PlainFolder::LoadByID(BackendImpl& backend, const std::string& id)
{
    backend.RequireAuthentication();

    nlohmann::json data(backend.GetFolder(id));

    return std::make_unique<PlainFolder>(backend, data, true, nullptr);
}

/*****************************************************/
PlainFolder::PlainFolder(BackendImpl& backend, Folder* parent) : 
    Folder(backend), mDebug(__func__,this)
{
    MDBG_INFO("()");

    mParent = parent;
}

/*****************************************************/
PlainFolder::PlainFolder(BackendImpl& backend, const nlohmann::json& data, Folder* parent) :
    Folder(backend, data), mDebug(__func__,this)
{
    MDBG_INFO("()");

    mParent = parent;

    MDBG_INFO("... ID:" << mId << " name:" << mName);
}

/*****************************************************/
PlainFolder::PlainFolder(BackendImpl& backend, const nlohmann::json& data, bool haveItems, Folder* parent) :
    PlainFolder(backend, data, parent)
{
    std::string fsid; try
    {
        if (haveItems) Folder::LoadItemsFrom(data);

        data.at("filesystem").get_to(fsid);
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }

    mFsConfig = &FSConfig::LoadByID(backend, fsid);
}

/*****************************************************/
void PlainFolder::SubLoadItems()
{
    ITDBG_INFO("()");

    LoadItemsFrom(mBackend.GetFolder(GetID()));
}

/*****************************************************/
void PlainFolder::SubCreateFile(const std::string& name)
{
    ITDBG_INFO("(name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    std::unique_ptr<File> file;

    if (mBackend.GetOptions().cacheType == ConfigOptions::CacheType::NONE)
    {
        nlohmann::json data(mBackend.CreateFile(GetID(), name));
        file = std::make_unique<File>(mBackend, data, *this);
    }
    else file = std::make_unique<File>(mBackend, *this, name, *mFsConfig, // create later
        [&](const std::string& fname){ 
            return mBackend.CreateFile(GetID(), fname); },
        [&](const std::string& fname, const std::string& fdata){ 
            return mBackend.UploadFile(GetID(), fname, fdata); });

    mItemMap[file->GetName()] = std::move(file);
}

/*****************************************************/
void PlainFolder::SubCreateFolder(const std::string& name)
{
    ITDBG_INFO("(name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    nlohmann::json data(mBackend.CreateFolder(GetID(), name));

    std::unique_ptr<PlainFolder> folder(std::make_unique<PlainFolder>(mBackend, data, false, this));

    mItemMap[folder->GetName()] = std::move(folder);
}

/*****************************************************/
void PlainFolder::SubDelete()
{
    ITDBG_INFO("()");

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.DeleteFolder(GetID());
}

/*****************************************************/
void PlainFolder::SubRename(const std::string& newName, bool overwrite)
{
    ITDBG_INFO("(name:" << newName << ")");

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.RenameFolder(GetID(), newName, overwrite);
}

/*****************************************************/
void PlainFolder::SubMove(const std::string& parentID, bool overwrite)
{
    ITDBG_INFO("(parent:" << parentID << ")");

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.MoveFolder(GetID(), parentID, overwrite);
}

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders
