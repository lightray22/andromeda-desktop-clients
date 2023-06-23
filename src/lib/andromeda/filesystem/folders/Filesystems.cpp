#include <functional>
#include <nlohmann/json.hpp>

#include "Filesystems.hpp"
#include "Filesystem.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/*****************************************************/
Filesystems::Filesystems(BackendImpl& backend, Folder& parent) : 
    Folder(backend), mDebug(__func__,this)
{
    MDBG_INFO("()");

    backend.RequireAuthentication();

    mName = "Filesystems";
    mParent = &parent;
}

/*****************************************************/
void Filesystems::SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& thisLock)
{
    MDBG_INFO("()");

    const nlohmann::json data(mBackend.GetFilesystems());

    Folder::NewItemMap newItems;

    NewItemFunc newFilesystem { [&](const nlohmann::json& fsJ)->std::unique_ptr<Item> {
        return std::make_unique<Filesystem>(mBackend, fsJ, this); } };

    try
    {
        for (const nlohmann::json& fsJ : data)
        {
            newItems.emplace(std::piecewise_construct,
                std::forward_as_tuple(fsJ.at("name")),
                std::forward_as_tuple(fsJ, newFilesystem));
        }
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }

    SyncContents(newItems, itemsLocks, thisLock);
}

} // namespace Folders
} // namespace Filesystem
} // namespace Andromeda
