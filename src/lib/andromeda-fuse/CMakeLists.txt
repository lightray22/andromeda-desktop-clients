cmake_minimum_required(VERSION 3.16)
project(libandromeda-fuse VERSION 1.0.0)

include(../../../andromeda.cmake)

# build the andromeda-fuse library

add_library(libandromeda-fuse)
set_target_properties(libandromeda-fuse PROPERTIES PREFIX "")

set(SOURCE_FILES FuseAdapter.cpp FuseOperations.cpp FuseOptions.cpp)

target_sources(libandromeda-fuse PRIVATE ${SOURCE_FILES})
target_include_directories(libandromeda-fuse
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_options(libandromeda-fuse PRIVATE ${ANDROMEDA_CXX_WARNS} ${ANDROMEDA_CXX_OPTS})
target_compile_definitions(libandromeda-fuse PRIVATE _FILE_OFFSET_BITS=64)

install(TARGETS libandromeda-fuse
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# include/link libandromeda

if (NOT TARGET libandromeda)
    add_subdirectory(../andromeda lib/andromeda)
endif()

target_link_libraries(libandromeda-fuse PUBLIC libandromeda)

# include/link libfuse

option(LIBFUSE2 "Force using FUSE 2.x" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(FUSE REQUIRED)

if (${FUSE_MAJOR_VERSION} MATCHES 2)
    target_compile_definitions(libandromeda-fuse PRIVATE LIBFUSE2=1)
endif()

target_include_directories(libandromeda-fuse PRIVATE ${FUSE_INCLUDE_DIR})
target_link_libraries(libandromeda-fuse INTERFACE ${FUSE_LIBRARY})
