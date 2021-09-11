
SET(FUSE_HEADER_NAMES fuse3/fuse.h)
SET(FUSE_HEADER_PATHS /usr/include /usr/local/include)

FIND_PATH(FUSE_INCLUDE_DIR NAMES ${FUSE_HEADER_NAMES} PATHS ${FUSE_HEADER_PATHS})

SET(FUSE_LIBRARY_NAMES fuse3)
SET(FUSE_LIBRARY_PATHS /lib64 /lib /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib /usr/lib/x86_64-linux-gnu)

SET(FUSE_NAMES fuse3)
FIND_LIBRARY(FUSE_LIBRARIES NAMES ${FUSE_LIBRARY_NAMES} PATHS {$FUSE_LIBRARY_PATHS})

include("FindPackageHandleStandardArgs")
find_package_handle_standard_args("FUSE" DEFAULT_MSG
    FUSE_INCLUDE_DIR FUSE_LIBRARIES)
        
mark_as_advanced(FUSE_INCLUDE_DIR FUSE_LIBRARIES)