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
        -pedantic -pedantic-errors -Wpedantic
        -Wno-unused-parameter # NO unused parameter
        -Wcast-align
        -Wcast-qual 
        -Wconversion 
        -Wdouble-promotion
        -Wfloat-equal
        -Wformat=2 
        -Wimplicit-fallthrough
        -Wnon-virtual-dtor 
        -Wold-style-cast
        -Woverloaded-virtual 
        -Wsign-conversion
        # -Wshadow TODO
    )

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        list(APPEND ANDROMEDA_WARNINGS 
            -Wduplicated-branches
            -Wduplicated-cond
            -Wlogical-op 
            -Wmisleading-indentation
            -Wnull-dereference
        )
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        list(APPEND ANDROMEDA_WARNINGS 
            -Wnewline-eof
        )
    endif()
endif()
