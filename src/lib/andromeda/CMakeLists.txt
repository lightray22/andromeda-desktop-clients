cmake_minimum_required(VERSION 3.16)
project(libandromeda VERSION 1.0.0)

include(../../../andromeda.cmake)

if (BUILD_TESTING)
    add_subdirectory(_tests)
endif()

# build the Andromeda library

add_library(andromeda)

set(SOURCE_FILES 
    Backend.cpp
    BackendOptions.cpp
    Config.cpp
    CLIRunner.cpp
    HTTPRunner.cpp
    HTTPRunnerOptions.cpp
    Utilities.cpp
    )

target_sources(andromeda PRIVATE ${SOURCE_FILES})
target_include_directories(andromeda
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_options(andromeda PRIVATE ${ANDROMEDA_WARNINGS})

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(andromeda PRIVATE -Wno-psabi)
endif()

install(TARGETS andromeda
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_subdirectory(filesystem)

# include/link libhttplib

if (APPLE)
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl@1.1)
endif()

set(HTTPLIB_COMPILE True)
set(HTTPLIB_REQUIRE_OPENSSL True)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE False)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE False)

FetchContent_Declare(cpp-httplib
    GIT_REPOSITORY  https://github.com/yhirose/cpp-httplib.git
    GIT_TAG         v0.11.3
    GIT_PROGRESS    true)
FetchContent_MakeAvailable(cpp-httplib)

target_link_libraries(andromeda PUBLIC httplib)

# include/link reproc++

set(REPROC++ ON)
set(REPROC_INSTALL OFF)

FetchContent_Declare(reproc
    GIT_REPOSITORY  https://github.com/stingray-11/reproc
    GIT_TAG         c1edf03 # master
    GIT_PROGRESS    true)
FetchContent_MakeAvailable(reproc)

target_link_libraries(andromeda PRIVATE reproc++)

# include nlohmann (header-only)

set(JSON_ImplicitConversions "OFF")
set(JSON_BuildTests OFF CACHE INTERNAL "")

FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY  https://github.com/nlohmann/json.git
    GIT_TAG         v3.11.2
    GIT_PROGRESS    true)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(andromeda PUBLIC nlohmann_json)
