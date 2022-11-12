cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# include and setup FetchContent

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

# define compiler warnings

# resource for custom warning codes
# https://github.com/cpp-best-practices/cppbestpractices/blob/master/02-Use_the_Tools_Available.md

if (MSVC)
    set(ANDROMEDA_WARNINGS /W4 /WX /permissive-
        /wd4100 # NO unreferenced formal parameter
        /wd4101 # NO unreferenced local variable
        /w14242 /w14254 /w14263 /w14265
        /w14287 /we4289 /w14296 /w14311
        /w14545 /w14546 /w14547 /w14549
        /w14555 /w14619 /w14640 /w14826
        /w14905 /w14906 /w14928
    )
else()
    set(ANDROMEDA_WARNINGS -Wall -Wextra -Werror
        -pedantic -pedantic-errors
        -Wno-unused-parameter # NO unused parameter
        # -Wshadow TODO
        -Wnon-virtual-dtor -Wold-style-cast
        -Wcast-align -Woverloaded-virtual -Wpedantic
        -Wconversion -Wdouble-promotion
        # -Wsign-conversion TODO
            -Wno-sign-conversion
        -Wformat=2 -Wimplicit-fallthrough
        -Wcast-qual -Wfloat-equal -Wcast-align
    )

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        list(APPEND ANDROMEDA_WARNINGS 
            -Wmisleading-indentation -Wduplicated-cond
            -Wduplicated-branches -Wlogical-op 
            -Wnull-dereference
        )
    endif()
endif()
