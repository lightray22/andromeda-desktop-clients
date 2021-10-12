cmake_minimum_required(VERSION 3.18)
project(andromeda-fuse-client VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SOURCE_FILES main.cpp FuseWrapper.cpp Options.cpp)
add_executable(andromeda-fuse ${SOURCE_FILES})

target_compile_options(andromeda-fuse PRIVATE -Wall -Wextra 
    -Wno-unused-parameter -Wno-missing-field-initializers)

add_subdirectory(andromeda)
target_link_libraries(andromeda-fuse PRIVATE andromeda)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

option(USE_FUSE2 "Use FUSE 2.x" OFF)

if (APPLE)
    SET(USE_FUSE2 ON)
endif()

find_package(FUSE REQUIRED)

target_link_libraries(andromeda-fuse PRIVATE ${FUSE_LIBRARIES})
target_include_directories(andromeda-fuse PRIVATE ${FUSE_INCLUDE_DIR})

target_compile_definitions(andromeda-fuse PRIVATE USE_FUSE2=$<BOOL:${USE_FUSE2}>)