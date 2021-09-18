#include "SuperRoot.hpp"
#include "../../Backend.hpp"

/*****************************************************/
SuperRoot::SuperRoot(Backend& backend) :
    BaseFolder(backend), debug("SuperRoot",this)
{
    debug << __func__ << "()"; debug.Out();

    this->name = "SUPERROOT";
};