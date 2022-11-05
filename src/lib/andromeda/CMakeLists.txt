cmake_minimum_required(VERSION 3.16)
project(libandromeda VERSION 1.0.0)

include(../../../CMakeLists.inc.txt)

# build the Andromeda library

add_library(andromeda)

set(SOURCE_FILES 
    Backend.cpp
    Config.cpp
    FSConfig.cpp
    CLIRunner.cpp
    HTTPRunner.cpp
    Utilities.cpp
    )

target_sources(andromeda PRIVATE ${SOURCE_FILES})
target_include_directories(andromeda
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_options(andromeda PRIVATE ${ANDROMEDA_WARNINGS})

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(andromeda PRIVATE -Wno-psabi)
endif()

add_subdirectory(fsitems)

# include/link openssl

if (APPLE) # TODO needed?
#    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl@1.1)
#    target_include_directories(andromeda PRIVATE ${OPENSSL_ROOT_DIR}/include)
endif()

find_package(OpenSSL REQUIRED)
target_link_libraries(andromeda PUBLIC OpenSSL::SSL)
target_link_libraries(andromeda PUBLIC OpenSSL::Crypto)

# include/link libhttplib

set(HTTPLIB_COMPILE True)
set(HTTPLIB_REQUIRE_OPENSSL True)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE False)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE False)

add_subdirectory(thirdparty/cpp-httplib)
target_include_directories(andromeda PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/cpp-httplib/out)
target_link_libraries(andromeda INTERFACE httplib)

# include/link reproc++

set(REPROC++ ON)
add_subdirectory(thirdparty/reproc)
target_link_libraries(andromeda PUBLIC reproc++)

# include nlohmann (header-only)

set(JSON_ImplicitConversions "OFF")
set(JSON_BuildTests OFF CACHE INTERNAL "")

add_subdirectory(thirdparty/json)
target_link_libraries(andromeda PUBLIC nlohmann_json::nlohmann_json)
