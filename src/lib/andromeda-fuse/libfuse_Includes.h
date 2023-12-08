#ifndef A2FUSE_FUSEINCLUDES_H_
#define A2FUSE_FUSEINCLUDES_H_

#if WIN32
    #define FUSE_USE_VERSION 35
    #pragma warning(push)
    #pragma warning(disable:4232)
        #include <fuse3/fuse.h>
    #pragma warning(pop)
#elif LIBFUSE2
    #define FUSE_USE_VERSION 26
    #include <fuse.h>
#ifndef OPENBSD
    #include <fuse_lowlevel.h>
#endif // !OPENBSD
#else
    #define FUSE_USE_VERSION 35
    #include <fuse3/fuse.h>
    #include <fuse3/fuse_lowlevel.h>
#endif // WIN32, LIBFUSE2

enum { FUSE_SUCCESS = 0 };

#if WIN32
    #define mode_t fuse_mode_t
    #define off_t fuse_off_t
    #define gid_t fuse_gid_t
    #define uid_t fuse_uid_t
    #define stat fuse_stat
    #define statvfs fuse_statvfs
    #define timespec fuse_timespec
    
    #define S_IRUSR 0400
    #define S_IWUSR 0200
    #define S_IXUSR 0100
    #define S_IRGRP 0040
    #define S_IWGRP 0020
    #define S_IXGRP 0010
    #define S_IROTH 0004
    #define S_IWOTH 0002
    #define S_IXOTH 0001
    #define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
    #define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)
    #define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)
#endif // WIN32

#endif // A2FUSE_FUSEINCLUDES_H_

