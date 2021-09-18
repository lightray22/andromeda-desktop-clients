#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>

#include "A2Fuse.hpp"

BaseFolder* A2Fuse::root = nullptr;

Debug A2Fuse::debug("A2Fuse");

void A2Fuse::Start(BaseFolder& root)
{
    debug << __func__ << "()"; debug.Out();
    
    A2Fuse::root = &root;
}
