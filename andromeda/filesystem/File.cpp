#include "File.hpp"
#include "Backend.hpp"

/*****************************************************/
File::File(Backend& backend) : 
    Item(backend), debug("File",this)
{
    debug << __func__ << "()"; debug.Info();
};
