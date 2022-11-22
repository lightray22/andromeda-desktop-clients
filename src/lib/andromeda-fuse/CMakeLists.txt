cmake_minimum_required(VERSION 3.16)
project(libandromeda-fuse VERSION 1.0.0)

include(../../../andromeda.cmake)

# build the andromeda-fuse library

add_library(andromeda-fuse)

set(SOURCE_FILES FuseAdapter.cpp FuseOperations.cpp)

target_sources(andromeda-fuse PRIVATE ${SOURCE_FILES})
target_include_directories(andromeda-fuse
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_options(andromeda-fuse PRIVATE ${ANDROMEDA_WARNINGS}
    -D_FILE_OFFSET_BITS=64)

install(TARGETS andromeda-fuse
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# include/link libandromeda

if (NOT TARGET andromeda)
    add_subdirectory(../andromeda lib/andromeda)
endif()

target_link_libraries(andromeda-fuse PUBLIC andromeda)

# include/link libfuse

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(FUSE REQUIRED)

if (${FUSE_MAJOR_VERSION} MATCHES 2)
    target_compile_definitions(andromeda-fuse PRIVATE LIBFUSE2=1)
endif()

target_include_directories(andromeda-fuse PRIVATE ${FUSE_INCLUDE_DIR})
target_link_libraries(andromeda-fuse INTERFACE ${FUSE_LIBRARY})
