#include <nlohmann/json.hpp>

#include "Adopted.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/filesystem/Folder.hpp"
using Andromeda::Filesystem::Folder;

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/*****************************************************/
Adopted::Adopted(BackendImpl& backend, Folder& parent) :
    PlainFolder(backend, &parent), mDebug(__func__,this)
{
    MDBG_INFO("()");

    mName = "Adopted by others";
}

/*****************************************************/
void Adopted::SubLoadItems(ItemLockMap& itemsLocks, const SharedLockW& thisLock)
{
    MDBG_INFO("()");

    LoadItemsFrom(mBackend.GetAdopted(), itemsLocks, thisLock);
}

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders
