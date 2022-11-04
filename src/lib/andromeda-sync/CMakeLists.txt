cmake_minimum_required(VERSION 3.16)
project(libandromeda-sync VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# build the andromeda-sync library

add_library(andromeda-sync)

#set(SOURCE_FILES FuseAdapter.cpp)

target_sources(andromeda-sync PRIVATE ${SOURCE_FILES})
target_include_directories(andromeda-sync
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(andromeda-sync PRIVATE -Wall -Wextra -Werror
    -Wno-unused-parameter -pedantic -pedantic-errors)

# include/link libandromeda

if (NOT TARGET andromeda)
    add_subdirectory(../andromeda lib/andromeda)
endif()

target_link_libraries(andromeda-sync PUBLIC andromeda)
