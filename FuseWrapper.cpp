#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>

#include "FuseWrapper.hpp"
#include "filesystem/BaseFolder.hpp"

BaseFolder* FuseWrapper::root = nullptr;
Debug FuseWrapper::debug("FuseWrapper");

/*****************************************************/
void FuseWrapper::Start(BaseFolder& root, const std::string& path)
{
    debug << __func__ << "(path:" << path << ")"; debug.Out();
    
    FuseWrapper::root = &root;
}
