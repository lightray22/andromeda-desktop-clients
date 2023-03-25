
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
        if (haveItems)
        {
            ItemLockMap lockMap; // empty, no existing
            const SharedLockW itemLock(GetWriteLock());
            LoadItemsFrom(data, lockMap, itemLock);
        }

        data.at("filesystem").get_to(fsid);
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }

    mFsConfig = &FSConfig::LoadByID(backend, fsid);
}

/*****************************************************/
void PlainFolder::SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& itemLock)
{
    ITDBG_INFO("()");

    LoadItemsFrom(mBackend.GetFolder(GetID()), itemsLocks, itemLock);
}

/*****************************************************/
void PlainFolder::LoadItemsFrom(const nlohmann::json& data, ItemLockMap& itemsLocks, const SharedLockW& itemLock)
{
    ITDBG_INFO("()");

    Folder::NewItemMap newItems;

    NewItemFunc newFile { [&](const nlohmann::json& fileJ)->std::unique_ptr<Item> {
        return std::make_unique<File>(mBackend, fileJ, *this); } };

    NewItemFunc newFolder { [&](const nlohmann::json& folderJ)->std::unique_ptr<Item> {
        return std::make_unique<Folders::PlainFolder>(mBackend, folderJ, false, this); } };

    try
    {
        for (const nlohmann::json& fileJ : data.at("files"))
            newItems.emplace(std::piecewise_construct,
                std::forward_as_tuple(fileJ.at("name")), std::forward_as_tuple(fileJ, newFile));

        for (const nlohmann::json& folderJ : data.at("folders"))
            newItems.emplace(std::piecewise_construct,
                std::forward_as_tuple(folderJ.at("name")), std::forward_as_tuple(folderJ, newFolder));
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }

    SyncContents(newItems, itemsLocks, itemLock);
}

/*****************************************************/
void PlainFolder::SubCreateFile(const std::string& name, const SharedLockW& itemLock)
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

    SharedLockR subLock { file->GetReadLock() };
    mItemMap[file->GetName(subLock)] = std::move(file);
}

/*****************************************************/
void PlainFolder::SubCreateFolder(const std::string& name, const SharedLockW& itemLock)
{
    ITDBG_INFO("(name:" << name << ")");

    if (isReadOnly()) throw ReadOnlyException();

    nlohmann::json data(mBackend.CreateFolder(GetID(), name));

    std::unique_ptr<PlainFolder> folder(std::make_unique<PlainFolder>(mBackend, data, false, this));

    SharedLockR subLock { folder->GetReadLock() };
    mItemMap[folder->GetName(subLock)] = std::move(folder);
}

/*****************************************************/
void PlainFolder::SubDelete(const SharedLockW& itemLock)
{
    ITDBG_INFO("()");

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.DeleteFolder(GetID());
}

/*****************************************************/
void PlainFolder::SubRename(const std::string& newName, const SharedLockW& itemLock, bool overwrite)
{
    ITDBG_INFO("(name:" << newName << ")");

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.RenameFolder(GetID(), newName, overwrite);
}

/*****************************************************/
void PlainFolder::SubMove(const std::string& parentID, const SharedLockW& itemLock, bool overwrite)
{
    ITDBG_INFO("(parent:" << parentID << ")");

    if (isReadOnly()) throw ReadOnlyException();

    mBackend.MoveFolder(GetID(), parentID, overwrite);
}

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders
