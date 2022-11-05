cmake_minimum_required(VERSION 3.16)
project(andromeda-fuse-cli VERSION 1.0.0)

include(../../../CMakeLists.inc.txt)

# build the andromeda-fuse-cli executable

set(SOURCE_FILES main.cpp Options.cpp)
add_executable(andromeda-fuse-cli ${SOURCE_FILES})

target_compile_options(andromeda-fuse-cli PRIVATE ${ANDROMEDA_WARNINGS})

# include/link libandromeda-fuse

if (NOT TARGET andromeda-fuse)
    add_subdirectory(../../lib/andromeda-fuse lib/andromeda-fuse)
endif()

if (NOT TARGET andromeda)
    add_subdirectory(../../lib/andromeda lib/andromeda)
endif()

target_link_libraries(andromeda-fuse-cli PRIVATE andromeda-fuse)
