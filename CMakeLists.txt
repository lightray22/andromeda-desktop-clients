cmake_minimum_required(VERSION 3.18)
project(andromeda-fuse-client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SOURCE_FILES main.cpp A2Fuse.cpp)
add_executable(a2fuse ${SOURCE_FILES})

add_subdirectory(libandromeda)
target_link_libraries(a2fuse PRIVATE libandromeda)
