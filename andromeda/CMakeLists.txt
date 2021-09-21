cmake_minimum_required(VERSION 3.18)
project(libandromeda)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(andromeda)

target_compile_options(andromeda PRIVATE "-Wall")

set(SOURCE_FILES 
    Backend.cpp
    Config.cpp
    CLIRunner.cpp
    HTTPRunner.cpp
    Utilities.cpp
    )

target_sources(andromeda PRIVATE ${SOURCE_FILES})
target_include_directories(andromeda
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(andromeda
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set(HTTPLIB_COMPILE True)
set(HTTPLIB_REQUIRE_OPENSSL True)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE False)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE False)
#set(HTTPLIB_REQUIRE_ZLIB True)

add_subdirectory(thirdparty/cpp-httplib)
target_include_directories(andromeda PRIVATE 
    "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/cpp-httplib/out")
target_link_libraries(andromeda INTERFACE httplib)

find_package(nlohmann_json 3 REQUIRED)
target_link_libraries(andromeda PRIVATE nlohmann_json)

add_subdirectory(filesystem)
