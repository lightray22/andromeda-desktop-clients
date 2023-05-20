cmake_minimum_required(VERSION 3.16)
project(libandromeda VERSION 1.0.0)

include(../../andromeda.cmake)

# build the Andromeda library

add_library(libandromeda STATIC)
andromeda_lib(libandromeda)

set(SOURCE_FILES 
    BaseOptions.cpp
    ConfigOptions.cpp
    Debug.cpp
    Utilities.cpp
    )

target_sources(libandromeda PRIVATE ${SOURCE_FILES})
target_include_directories(libandromeda
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

install(TARGETS libandromeda
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_subdirectory(backend)
add_subdirectory(filesystem)

include(3rdparty.cmake)
