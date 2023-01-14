cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(ANDROMEDA_VERSION "0.1-alpha")
set(ANDROMEDA_CXX_DEFS ANDROMEDA_VERSION="${ANDROMEDA_VERSION}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    list(APPEND ANDROMEDA_CXX_DEFS LINUX)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    list(APPEND ANDROMEDA_CXX_DEFS FREEBSD)
elseif (APPLE)
    list(APPEND ANDROMEDA_CXX_DEFS APPLE)
endif()

include(GNUInstallDirs)

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
    
    if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/_tests)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/_tests)
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(Catch2 PRIVATE -Wno-psabi)
    endif()
endif()

# define compiler warnings

# resource for custom warning codes
# https://github.com/cpp-best-practices/cppbestpractices/blob/master/02-Use_the_Tools_Available.md

if (MSVC)
    set(ANDROMEDA_CXX_WARNS /W4 /WX /permissive-
        /wd4100 # NO unreferenced formal parameter
        /wd4101 # NO unreferenced local variable
        /wd4702 # NO unreachable code (Qt)
        /w14242 /w14254 /w14263 /w14265
        /w14287 /we4289 /w14296 /w14311
        /w14545 /w14546 /w14547 /w14549
        /w14555 /w14619 /w14640 /w14826
        /w14905 /w14906 /w14928
    )

    # security options
    set(ANDROMEDA_CXX_OPTS)
    set(ANDROMEDA_LINK_OPTS 
        /NXCOMPAT /DYNAMICBASE
    )
else()
    set(ANDROMEDA_CXX_WARNS -Wall -Wextra -Werror
        -pedantic -pedantic-errors -Wpedantic
        -Wno-unused-parameter # NO unused parameter
        -Wcast-align
        -Wcast-qual 
        -Wconversion 
        -Wdouble-promotion
        -Wformat=2 
        -Wimplicit-fallthrough
        -Wnon-virtual-dtor 
        -Wold-style-cast
        -Woverloaded-virtual 
        -Wsign-conversion
        -Wshadow
    )
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        list(APPEND ANDROMEDA_CXX_WARNS 
            -Wduplicated-branches
            -Wduplicated-cond
            -Wlogical-op 
            -Wmisleading-indentation
            -Wnull-dereference # effective with -O3
            -Wno-psabi # ignore GCC 7.1 armhf ABI change
        )
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        list(APPEND ANDROMEDA_CXX_WARNS 
            -Wnewline-eof
        )
    endif()

    # https://wiki.ubuntu.com/ToolChain/CompilerFlags
    # https://developers.redhat.com/blog/2018/03/21/compiler-and-linker-flags-gcc

    # security options
    set(ANDROMEDA_CXX_OPTS
        $<IF:$<CONFIG:Debug>,,-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3>
        -fstack-protector-strong --param=ssp-buffer-size=4
        -D_GLIBCXX_ASSERTIONS -fexceptions
    )
    if (NOT APPLE)
        set(ANDROMEDA_LINK_OPTS
            -Wl,-z,relro -Wl,-z,now
            -Wl,-z,noexecstack
        )
    endif()

    option(WITHOUT_PIE "Disable position independent executable" OFF)

    if (NOT ${WITHOUT_PIE})
        list(APPEND ANDROMEDA_CXX_OPTS -fPIE)
        list(APPEND ANDROMEDA_LINK_OPTS -Wl,-pie -pie)
    endif()
endif()
