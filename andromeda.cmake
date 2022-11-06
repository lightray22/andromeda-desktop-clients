cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (MSVC)
#    set(ANDROMEDA_WARNINGS /W4 /WX) # TODO enable me
else()
    set(ANDROMEDA_WARNINGS -Wall -Wextra -Werror
        -Wno-unused-parameter -pedantic -pedantic-errors)
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

option(BUILD_TESTING "Build unit tests" OFF)

if (BUILD_TESTING)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.1.1)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()
