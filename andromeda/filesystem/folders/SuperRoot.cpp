#include "SuperRoot.hpp"
#include "Backend.hpp"
#include "Filesystems.hpp"

/*****************************************************/
SuperRoot::SuperRoot(Backend& backend) : 
    Folder(backend), debug("SuperRoot",this)
{
    debug << __func__ << "()"; debug.Info();

    backend.RequireAuthentication();
}

/*****************************************************/
void SuperRoot::LoadItems()
{
    debug << __func__ << "()"; debug.Info();

    std::unique_ptr<Filesystems> filesystems(std::make_unique<Filesystems>(backend, *this));

    this->itemMap[filesystems->GetName()] = std::move(filesystems);
}
