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
    PlainFolder(backend, nullptr, &parent), mDebug("Adopted",this)
{
    mDebug << __func__ << "()"; mDebug.Info();

    mName = "Adopted by others";
}

/*****************************************************/
void Adopted::LoadItems()
{
    mDebug << __func__ << "()"; mDebug.Info();

    Folder::LoadItemsFrom(mBackend.GetAdopted());
}

} // namespace Andromeda
} // namespace Filesystem
} // namespace Folders
