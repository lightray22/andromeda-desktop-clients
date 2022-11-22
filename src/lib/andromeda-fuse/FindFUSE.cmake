
if (WIN32)
    SET(FUSE_HEADER_NAMES fuse3/fuse.h)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        SET(FUSE_LIBRARY_NAMES winfsp-x64)
    elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
        SET(FUSE_LIBRARY_NAMES winfsp-x86)
    endif()
else()
    SET(FUSE_HEADER_NAMES fuse3/fuse.h fuse/fuse.h)
    SET(FUSE_LIBRARY_NAMES fuse3 fuse)
endif()

if (WIN32)
    if (NOT FUSE_DIR)
        SET(FUSE_DIR "C:/Program Files (x86)/WinFsp")
    endif()
    SET(FUSE_HEADER_PATHS "${FUSE_DIR}/inc")
    SET(FUSE_LIBRARY_PATHS "${FUSE_DIR}/lib")
else()
    SET(FUSE_HEADER_PATHS /usr/include /usr/local/include)
    SET(FUSE_LIBRARY_PATHS /lib64 /lib /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib /usr/lib/x86_64-linux-gnu)
endif()

FIND_PATH(FUSE_INCLUDE_DIR NAMES ${FUSE_HEADER_NAMES} PATHS ${FUSE_HEADER_PATHS})
FIND_LIBRARY(FUSE_LIBRARY NAMES ${FUSE_LIBRARY_NAMES} PATHS ${FUSE_LIBRARY_PATHS})

if (${FUSE_LIBRARY} MATCHES fuse3 OR ${FUSE_LIBRARY} MATCHES winfsp)
   set(FUSE_MAJOR_VERSION 3)
else()
   set(FUSE_MAJOR_VERSION 2)
endif()

include("FindPackageHandleStandardArgs")
find_package_handle_standard_args("FUSE" DEFAULT_MSG
    FUSE_INCLUDE_DIR FUSE_LIBRARY)

mark_as_advanced(FUSE_INCLUDE_DIR FUSE_LIBRARY)
