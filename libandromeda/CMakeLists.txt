cmake_minimum_required(VERSION 3.18)
project(libandromeda)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(libandromeda)

set(SOURCE_FILES Backend.cpp
                CLIBackend.cpp
                HTTPBackend.cpp
                Utilities.cpp
                )

target_sources(libandromeda PRIVATE ${SOURCE_FILES})

target_include_directories(libandromeda
          INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(filesystem)
