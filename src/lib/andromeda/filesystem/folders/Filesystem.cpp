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

    return std::make_unique<Filesystem>(backend, data);
}

/*****************************************************/
Filesystem::Filesystem(BackendImpl& backend, const nlohmann::json& data, Folder* parent) :
    PlainFolder(backend), mDebug("Filesystem",this) 
{
    mDebug << __func__ << "()"; mDebug.Info();

    Initialize(data); mParent = parent;

    mFsid = mId; mId = "";

    mFsConfig = &FSConfig::LoadByID(backend, mFsid);
}

/*****************************************************/
const std::string& Filesystem::GetID()
{
    if (mId.empty()) LoadItems(); // populates ID

    return mId;
}

/*****************************************************/
void Filesystem::LoadItems()
{
    mDebug << __func__ << "()"; mDebug.Info();

    const nlohmann::json data(mBackend.GetFSRoot(mFsid));

    try
    {
        data.at("id").get_to(mId); // late load
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }

    Folder::LoadItemsFrom(data);
}

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders
