cmake_minimum_required(VERSION 3.16)
project(libandromeda-fuse)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# build the andromeda-fuse library

add_library(andromeda-fuse)

set(SOURCE_FILES FuseWrapper.cpp)

target_sources(andromeda-fuse PRIVATE ${SOURCE_FILES})
target_include_directories(andromeda-fuse
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(andromeda-fuse PRIVATE -Wall -Wextra 
    -Wno-unused-parameter -Wno-missing-field-initializers 
    -D_FILE_OFFSET_BITS=64)

# include/link libandromeda

add_subdirectory(../andromeda andromeda)
target_link_libraries(andromeda-fuse PRIVATE andromeda)

# include/link libfuse

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

option(USE_FUSE2 "Use FUSE 2.x" OFF)

if (APPLE)
    SET(USE_FUSE2 ON)
endif()

find_package(FUSE REQUIRED)

target_include_directories(andromeda-fuse PRIVATE ${FUSE_INCLUDE_DIR})
target_link_libraries(andromeda-fuse INTERFACE ${FUSE_LIBRARIES})

target_compile_definitions(andromeda-fuse PRIVATE USE_FUSE2=$<BOOL:${USE_FUSE2}>)
