cmake_minimum_required(VERSION 3.18)
project(andromeda-fuse-client VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


set(SOURCE_FILES main.cpp A2Fuse.cpp Options.cpp)
add_executable(a2fuse ${SOURCE_FILES})

add_subdirectory(andromeda)
target_link_libraries(a2fuse PRIVATE andromeda)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(FUSE REQUIRED)
target_link_libraries(a2fuse PRIVATE ${FUSE_LIBRARIES})
target_include_directories(a2fuse PRIVATE ${FUSE_INCLUDE_DIR})