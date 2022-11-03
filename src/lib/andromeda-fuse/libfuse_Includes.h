
#if USE_FUSE2
#define FUSE_USE_VERSION 26
#include <fuse/fuse.h>
#include <fuse/fuse_lowlevel.h>
#else
#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>
#include <fuse3/fuse_lowlevel.h>
#endif

#define FUSE_SUCCESS 0
