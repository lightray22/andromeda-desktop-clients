
#include <nlohmann/json.hpp>

#include "PlainFolder.hpp"
#include "andromeda/ConfigOptions.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/backend/RunnerInput.hpp"
using Andromeda::Backend::WriteFunc;
#include "andromeda/filesystem/FSConfig.hpp"
#include "andromeda/filesystem/File.hpp"
using Andromeda::Filesystem::File;

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/*****************************************************/
std::unique_ptr<PlainFolder> PlainFolder::LoadByID(BackendImpl& backend, const std::string& id)
{
    backend.RequireAuthentication();

    const nlohmann::json data(backend.GetFolder(id));

    return std::make_unique<PlainFolder>(backend, data, true, nullptr);
}

/*****************************************************/
PlainFolder::PlainFolder(BackendImpl& backend, Folder* parent) : 
    Folder(backend), mDebug(__func__,this)
{
    MDBG_INFO("(2)");
    mParent = parent;
}

/*****************************************************/
PlainFolder::PlainFolder(BackendImpl& backend, const nlohmann::json& data, Folder* parent) :
    Folder(backend, data), mDebug(__func__,this)
{
    MDBG_INFO("(3)");
    mParent = parent;

    MDBG_INFO("... ID:" << mId << " name:" << mName);
}

/*****************************************************/
PlainFolder::PlainFolder(BackendImpl& backend, const nlohmann::json& data, bool haveItems, Folder* parent) :
    PlainFolder(backend, data, parent)
{
    MDBG_INFO("(4)");

    std::string fsid; try
    {
        if (haveItems)
        {
            ItemLockMap lockMap; // empty, no existing
            LoadItemsFrom(data, lockMap, GetWriteLock());
        }

        data.at("filesystem").get_to(fsid);
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }

    mFsConfig = &FSConfig::LoadByID(backend, fsid);
}

/*****************************************************/
void PlainFolder::SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& thisLock)
{
    ITDBG_INFO("()");

    LoadItemsFrom(mBackend.GetFolder(GetID()), itemsLocks, thisLock);
}

/*****************************************************/
void PlainFolder::LoadItemsFrom(const nlohmann::json& data, ItemLockMap& itemsLocks, const SharedLockW& thisLock)
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
                std::forward_as_tuple(fileJ.at("name")), 
                std::forward_as_tuple(fileJ, newFile));

        for (const nlohmann::json& folderJ : data.at("folders"))
            newItems.emplace(std::piecewise_construct,
                std::forward_as_tuple(folderJ.at("name")), 
                std::forward_as_tuple(folderJ, newFolder));
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }

    SyncContents(newItems, itemsLocks, thisLock);
}

/*****************************************************/
void PlainFolder::SubCreateFile(const std::string& name, const SharedLockW& thisLock)
{
    ITDBG_INFO("(name:" << name << ")");

    if (isReadOnlyFS()) throw ReadOnlyFSException();

    std::unique_ptr<File> file;

    if (mBackend.GetOptions().cacheType == ConfigOptions::CacheType::NONE)
    {
        const nlohmann::json data(mBackend.CreateFile(GetID(), name));
        file = std::make_unique<File>(mBackend, data, *this);
    }
    else file = std::make_unique<File>(mBackend, *this, name, *mFsConfig, // create later
        [&](const std::string& fname){ 
            return mBackend.CreateFile(GetID(), fname); },
        [&](const std::string& fname, const WriteFunc& ffunc, bool oneshot){ 
            return mBackend.UploadFile(GetID(), fname, ffunc, oneshot); });

    const SharedLockR subLock { file->GetReadLock() };
    mItemMap[file->GetName(subLock)] = std::move(file);
}

/*****************************************************/
void PlainFolder::SubCreateFolder(const std::string& name, const SharedLockW& thisLock)
{
    ITDBG_INFO("(name:" << name << ")");

    if (isReadOnlyFS()) throw ReadOnlyFSException();

    const nlohmann::json data(mBackend.CreateFolder(GetID(), name));

    std::unique_ptr<PlainFolder> folder(std::make_unique<PlainFolder>(mBackend, data, false, this));

    const SharedLockR subLock { folder->GetReadLock() };
    mItemMap[folder->GetName(subLock)] = std::move(folder);
}

/*****************************************************/
void PlainFolder::SubDelete(const DeleteLock& deleteLock)
{
    ITDBG_INFO("()");

    if (isReadOnlyFS()) throw ReadOnlyFSException();

    mBackend.DeleteFolder(GetID());
}

/*****************************************************/
void PlainFolder::SubRename(const std::string& newName, const SharedLockW& thisLock, bool overwrite)
{
    ITDBG_INFO("(name:" << newName << ")");

    if (isReadOnlyFS()) throw ReadOnlyFSException();

    mBackend.RenameFolder(GetID(), newName, overwrite);
}

/*****************************************************/
void PlainFolder::SubMove(const std::string& parentID, const SharedLockW& thisLock, bool overwrite)
{
    ITDBG_INFO("(parent:" << parentID << ")");

    if (isReadOnlyFS()) throw ReadOnlyFSException();

    mBackend.MoveFolder(GetID(), parentID, overwrite);
}

} // namespace Folders
} // namespace Filesystem
} // namespace Andromeda
