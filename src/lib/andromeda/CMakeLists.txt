cmake_minimum_required(VERSION 3.16)
project(libandromeda VERSION 1.0.0)

include(../../andromeda.cmake)

# build the Andromeda library

add_library(libandromeda STATIC)
set_target_properties(libandromeda PROPERTIES PREFIX "")

set(SOURCE_FILES 
    BaseOptions.cpp
    ConfigOptions.cpp
    Debug.cpp
    Utilities.cpp
    )

target_sources(libandromeda PRIVATE ${SOURCE_FILES})
target_include_directories(libandromeda
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_options(libandromeda PRIVATE ${ANDROMEDA_CXX_WARNS} ${ANDROMEDA_CXX_OPTS})
target_compile_definitions(libandromeda PRIVATE ${ANDROMEDA_CXX_DEFS})

install(TARGETS libandromeda
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_subdirectory(backend)
add_subdirectory(filesystem)

include(3rdparty.cmake)
