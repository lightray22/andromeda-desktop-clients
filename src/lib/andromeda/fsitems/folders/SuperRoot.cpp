#include <nlohmann/json.hpp>

#include "andromeda/Backend.hpp"
#include "Adopted.hpp"
#include "Filesystems.hpp"
#include "SuperRoot.hpp"

namespace Andromeda {
namespace FSItems {
namespace Folders {

/*****************************************************/
SuperRoot::SuperRoot(Backend& backend) : 
    Folder(backend), debug("SuperRoot",this)
{
    debug << __func__ << "()"; debug.Info();

    backend.RequireAuthentication();

    this->name = "SuperRoot";
}

/*****************************************************/
void SuperRoot::LoadItems()
{
    if (HaveItems()) return; // never refresh

    debug << __func__ << "()"; debug.Info();

    std::unique_ptr<Adopted> adopted(std::make_unique<Adopted>(backend, *this));
    this->itemMap[adopted->GetName()] = std::move(adopted);

    std::unique_ptr<Filesystems> filesystems(std::make_unique<Filesystems>(backend, *this));
    this->itemMap[filesystems->GetName()] = std::move(filesystems);
}

} // namespace Andromeda
} // namespace FSItems
} // namespace Folders
