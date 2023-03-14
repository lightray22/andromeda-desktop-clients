cmake_minimum_required(VERSION 3.16)
project(libandromeda VERSION 1.0.0)

include(../../../andromeda.cmake)

# build the Andromeda library

add_library(libandromeda)
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
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_subdirectory(backend)
add_subdirectory(filesystem)

# include/link libhttplib

if (APPLE)
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
endif()

set(HTTPLIB_COMPILE True)
set(HTTPLIB_REQUIRE_OPENSSL True)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE False)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE False)

set(DEPS_BASEURL "https://github.com" CACHE STRING "Base URL for git dependencies")
# example to set up a local git repo for testing to avoid cloning from github repeatedly
# git clone https://github.com/nlohmann/json.git --mirror; cd json.git; git --bare update-server-info; mv hooks/post-update.sample hooks/post-update

FetchContent_Declare(cpp-httplib
    GIT_REPOSITORY  ${DEPS_BASEURL}/yhirose/cpp-httplib.git
    GIT_TAG         f2f4728 # v0.12.1
    GIT_PROGRESS    true)
FetchContent_MakeAvailable(cpp-httplib)

target_compile_options(httplib PRIVATE ${ANDROMEDA_CXX_OPTS}) # hardening

target_link_libraries(libandromeda PUBLIC httplib)

# include/link reproc++

set(REPROC++ ON)
set(REPROC_INSTALL OFF)

FetchContent_Declare(reproc
    GIT_REPOSITORY  ${DEPS_BASEURL}/stingray-11/reproc.git
    GIT_TAG         c1edf03 # master
    GIT_PROGRESS    true)
FetchContent_MakeAvailable(reproc)

target_compile_options(reproc PRIVATE ${ANDROMEDA_CXX_OPTS}) # hardening
target_compile_options(reproc++ PRIVATE ${ANDROMEDA_CXX_OPTS}) # hardening

target_link_libraries(libandromeda PRIVATE reproc++)

# include nlohmann (header-only)

set(JSON_ImplicitConversions "OFF")
set(JSON_BuildTests OFF CACHE INTERNAL "")

FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY  ${DEPS_BASEURL}/nlohmann/json.git
    GIT_TAG         bc889af # v3.11.2
    GIT_PROGRESS    true)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(libandromeda PUBLIC nlohmann_json)
