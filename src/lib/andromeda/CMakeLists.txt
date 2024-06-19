cmake_minimum_required(VERSION 3.16)
project(libandromeda VERSION 1.0.0)

include(../../andromeda.cmake)

# build the Andromeda library

set(SOURCE_FILES 
    BaseOptions.cpp
    ConfigOptions.cpp
    Crypto.cpp
    Debug.cpp
    PlatformUtil.cpp
    SecureBuffer.cpp
    StringUtil.cpp
    )
andromeda_lib(libandromeda "${SOURCE_FILES}")

target_include_directories(libandromeda
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux") # needed for armv6
    target_link_libraries(libandromeda PUBLIC atomic)
endif()

install(TARGETS libandromeda
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_subdirectory(backend)
add_subdirectory(database)
add_subdirectory(filesystem)


# include/link libhttplib

if (APPLE)
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
endif()

set(HTTPLIB_COMPILE True)
set(HTTPLIB_INSTALL False)
set(HTTPLIB_REQUIRE_OPENSSL True)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE False)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE False)

set(DEPS_BASEURL "https://github.com" CACHE STRING "Base URL for git dependencies")
# example to set up a local git repo for testing to avoid cloning from github repeatedly
# git clone https://github.com/nlohmann/json.git --mirror; cd json.git; git --bare update-server-info; mv hooks/post-update.sample hooks/post-update

FetchContent_Declare(cpp-httplib
    GIT_REPOSITORY  ${DEPS_BASEURL}/yhirose/cpp-httplib.git
    GIT_TAG         8438df4 # v0.16.0
    GIT_PROGRESS    true)
FetchContent_MakeAvailable(cpp-httplib)

target_compile_options(httplib PRIVATE ${ANDROMEDA_CXX_OPTS}) # hardening

target_link_libraries(libandromeda PUBLIC httplib)

# include/link reproc++

set(REPROC++ ON)
set(REPROC_INSTALL OFF)

FetchContent_Declare(reproc
    GIT_REPOSITORY  ${DEPS_BASEURL}/stingray-11/reproc.git
    #GIT_REPOSITORY ${DEPS_BASEURL}/DaanDeMeyer/reproc.git
    GIT_TAG         0b10db3 # main
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
    GIT_TAG         9cca280 # v3.11.3
    GIT_PROGRESS    true)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(libandromeda PUBLIC nlohmann_json)

# include/link libsqlite3

if (WIN32 AND DEFINED ENV{SQLITE3_ROOT_DIR})
    set(CMAKE_PREFIX_PATH $ENV{SQLITE3_ROOT_DIR})
endif()

include(FindSQLite3)
find_package(SQLite3 REQUIRED)

target_include_directories(libandromeda PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(libandromeda INTERFACE ${SQLite3_LIBRARIES})

# include/link libsodium

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(Sodium REQUIRED)

target_link_libraries(libandromeda PUBLIC sodium)
