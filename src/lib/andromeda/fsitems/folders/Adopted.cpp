#include <nlohmann/json.hpp>

#include "Adopted.hpp"
#include "andromeda/Backend.hpp"
#include "andromeda/fsitems/Folder.hpp"

namespace Andromeda {
namespace FSItems {
namespace Folders {

/*****************************************************/
Adopted::Adopted(Backend& backend, Folder& parent) :
    PlainFolder(backend, nullptr, &parent), debug("Adopted",this)
{
    debug << __func__ << "()"; debug.Info();

    this->name = "Adopted by others";
}

/*****************************************************/
void Adopted::LoadItems()
{
    debug << __func__ << "()"; debug.Info();

    Folder::LoadItemsFrom(backend.GetAdopted());
}

} // namespace Andromeda
} // namespace FSItems
} // namespace Folders
