#include "SuperRoot.hpp"
#include "Backend.hpp"

/*****************************************************/
SuperRoot::SuperRoot(Backend& backend) : 
    RootFolder(backend), debug("SuperRoot",this)
{
    debug << __func__ << "()"; debug.Info();

    backend.RequireAuthentication();
}

/*****************************************************/
void SuperRoot::LoadItems()
{
    // TODO load from server
}
