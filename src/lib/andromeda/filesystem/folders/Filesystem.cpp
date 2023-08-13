#include "nlohmann/json.hpp"

#include "Filesystem.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/FSConfig.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/*****************************************************/
std::unique_ptr<Filesystem> Filesystem::LoadByID(BackendImpl& backend, const std::string& fsid)
{
    const nlohmann::json data(backend.GetFilesystem(fsid));

    return std::make_unique<Filesystem>(backend, data, nullptr);
}

/*****************************************************/
Filesystem::Filesystem(BackendImpl& backend, const nlohmann::json& data, Folder* parent) :
    PlainFolder(backend, data, parent), mFsid(mId), mDebug(__func__,this) 
{
    MDBG_INFO("()");

    mId = "";
    // our JSON ID is the FS ID
    // use mId for the root folder

    mFsConfig = &FSConfig::LoadByID(backend, mFsid);
}

/*****************************************************/
const std::string& Filesystem::GetID()
{
    const UniqueLock idLock(mIdMutex); // lazy-load the folder ID
    if (mId.empty()) LoadID(mBackend.GetFSRoot(mFsid), idLock);

    return mId;
}

/*****************************************************/
void Filesystem::LoadID(const nlohmann::json& data, const UniqueLock& idLock)
{
    try
    {
        data.at("id").get_to(mId);
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

/*****************************************************/
void Filesystem::SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& thisLock)
{
    ITDBG_INFO("()");

    const nlohmann::json data(mBackend.GetFSRoot(mFsid));

    { // lock scope
        const UniqueLock idLock(mIdMutex);
        if (mId.empty()) LoadID(data, idLock);
    }

    LoadItemsFrom(data, itemsLocks, thisLock);
}

} // namespace Folders
} // namespace Filesystem
} // namespace Andromeda
