#include <nlohmann/json.hpp>

#include "Filesystem.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/FSConfig.hpp"
using Andromeda::Filesystem::FSConfig;

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/*****************************************************/
std::unique_ptr<Filesystem> Filesystem::LoadByID(BackendImpl& backend, const std::string& fsid)
{
    backend.RequireAuthentication();

    const nlohmann::json data(backend.GetFilesystem(fsid));

    return std::make_unique<Filesystem>(backend, data, nullptr);
}

/*****************************************************/
Filesystem::Filesystem(BackendImpl& backend, const nlohmann::json& data, Folder* parent) :
    PlainFolder(backend, data, parent), mDebug(__func__,this) 
{
    MDBG_INFO("()");

    mFsid = mId; mId = "";
    // our JSON ID is the FS ID
    // use mId for the root folder

    mFsConfig = &FSConfig::LoadByID(backend, mFsid);
}

/*****************************************************/
const std::string& Filesystem::GetID()
{
    { // lock scope
        const SharedLockW itemLock(GetWriteLock());
        if (mId.empty()) LoadItems(itemLock, true); // populates ID
    }

    return mId;
}

/*****************************************************/
void Filesystem::SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& itemLock)
{
    ITDBG_INFO("()");

    const nlohmann::json data(mBackend.GetFSRoot(mFsid));

    try
    {
        data.at("id").get_to(mId); // have root folder ID now
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }

    LoadItemsFrom(data, itemsLocks, itemLock);
}

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders
