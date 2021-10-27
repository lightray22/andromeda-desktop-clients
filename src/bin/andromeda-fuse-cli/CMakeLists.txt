cmake_minimum_required(VERSION 3.18)
project(andromeda-fuse VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# build the andromeda-fuse-cli executable

set(SOURCE_FILES main.cpp Options.cpp)
add_executable(andromeda-fuse-cli ${SOURCE_FILES})

target_compile_options(andromeda-fuse-cli PRIVATE -Wall -Wextra -Wno-unused-parameter)

# include/link libandromeda-fuse

add_subdirectory(../../lib/andromeda-fuse andromeda-fuse)
target_link_libraries(andromeda-fuse-cli PRIVATE andromeda-fuse)
target_link_libraries(andromeda-fuse-cli PRIVATE andromeda)
