#include "SuperRoot.hpp"
#include "Backend.hpp"
#include "Adopted.hpp"
#include "Filesystems.hpp"

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
    debug << __func__ << "()"; debug.Info();

    std::unique_ptr<Adopted> adopted(std::make_unique<Adopted>(backend, *this));
    this->itemMap[adopted->GetName()] = std::move(adopted);

    std::unique_ptr<Filesystems> filesystems(std::make_unique<Filesystems>(backend, *this));
    this->itemMap[filesystems->GetName()] = std::move(filesystems);
}
