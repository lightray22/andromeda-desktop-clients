#include <nlohmann/json.hpp>

#include "Adopted.hpp"
#include "andromeda/Backend.hpp"
#include "andromeda/filesystem/Folder.hpp"

namespace Andromeda {
namespace Filesystem {
namespace Folders {

/*****************************************************/
Adopted::Adopted(Backend& backend, Folder& parent) :
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
