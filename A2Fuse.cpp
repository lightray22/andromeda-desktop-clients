
#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>

#include "A2Fuse.hpp"
#include "Backend.hpp"
#include "filesystem/BaseFolder.hpp"

Backend* A2Fuse::backend = nullptr;
BaseFolder* A2Fuse::folder = nullptr;

void A2Fuse::Start(Backend* backend, BaseFolder* folder)
{
    A2Fuse::backend = backend;
    A2Fuse::folder = folder;
}
