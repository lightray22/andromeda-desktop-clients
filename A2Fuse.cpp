#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>

#include "A2Fuse.hpp"

BaseFolder* A2Fuse::root = nullptr;

void A2Fuse::Start(BaseFolder& root)
{
    A2Fuse::root = &root;
}
