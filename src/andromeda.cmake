cmake_minimum_required(VERSION 3.16)

include(GNUInstallDirs)
include(CheckCXXCompilerFlag)

# this common file is to be included for each lib or bin target

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_EXTENSIONS False)
set(CMAKE_CXX_EXTENSIONS False)

set(ANDROMEDA_VERSION "0.1-alpha")
set(ANDROMEDA_CXX_DEFS 
    ANDROMEDA_VERSION="${ANDROMEDA_VERSION}"
    SYSTEM_NAME="${CMAKE_SYSTEM_NAME}"
    DEBUG=$<IF:$<CONFIG:Debug>,1,0>)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    list(APPEND ANDROMEDA_CXX_DEFS LINUX)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    list(APPEND ANDROMEDA_CXX_DEFS FREEBSD)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
    list(APPEND ANDROMEDA_CXX_DEFS OPENBSD)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
    list(APPEND ANDROMEDA_CXX_DEFS NETBSD)
elseif (APPLE)
    list(APPEND ANDROMEDA_CXX_DEFS APPLE)
endif()

# enable LTO (link time optimization) for all compilers
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# include and setup FetchContent

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

option(TESTS_CATCH2    "Build catch2 unit tests"        OFF)
option(TESTS_CLANGTIDY "Use clang-tidy static analysis" OFF)
option(TESTS_CPPCHECK  "Use cppcheck static analysis"   OFF)

if (TESTS_CATCH2)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY  https://github.com/catchorg/Catch2.git
        GIT_TAG         v3.7.0
        GIT_PROGRESS    true)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras) # Catch2WithMain

    FetchContent_Declare(trompeloeil # header-only
        GIT_REPOSITORY  https://github.com/rollbear/trompeloeil.git
        GIT_TAG         v47
        GIT_PROGRESS    true)
    FetchContent_MakeAvailable(trompeloeil)
    
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(Catch2 PRIVATE -Wno-psabi)
    endif()

    # andromeda bin/lib folders can contain _tests
    if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/_tests)
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/_tests)
    endif()
endif()

# define compiler warnings

# resource for custom warning codes
# https://github.com/cpp-best-practices/cppbestpractices/blob/master/02-Use_the_Tools_Available.md

option(ALLOW_WARNINGS "Allow building with warnings" OFF)

if (MSVC)
    set(ANDROMEDA_CXX_WARNS /W4 /permissive-
        /wd4100 # NO unreferenced formal parameter
        /wd4101 # NO unreferenced local variable
        /wd4702 # NO unreachable code (Qt)
        /w14242 /w14254 /w14263 /w14265
        /w14287 /we4289 /w14296 /w14311
        /w14545 /w14546 /w14547 /w14549
        /w14555 /w14619 /w14640 /w14826
        /w14905 /w14906 /w14928
    )

    if (NOT ${ALLOW_WARNINGS})
        list(APPEND ANDROMEDA_CXX_WARNS /WX)
    endif()

    # security options
    set(ANDROMEDA_CXX_OPTS)
    set(ANDROMEDA_LINK_OPTS 
        /NXCOMPAT /DYNAMICBASE
    )

else() # NOT MSVC

    ### ADD WARNINGS ###

    set(ANDROMEDA_CXX_WARNS -Wall -Wextra
        -pedantic -Wpedantic
        -Wno-unused-parameter # NO unused parameter
        -Wno-unused-function # NO unused function
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

    # if running clang-tidy, can't use GCC-specific options
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT ${TESTS_CLANGTIDY})
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

    if (NOT ${ALLOW_WARNINGS})
        list(APPEND ANDROMEDA_CXX_WARNS 
            -Werror -pedantic-errors)
    endif()

    ### ADD HARDENING ###

    # https://wiki.ubuntu.com/ToolChain/CompilerFlags
    # https://developers.redhat.com/blog/2018/03/21/compiler-and-linker-flags-gcc

    # security options
    set(ANDROMEDA_CXX_OPTS
        -fstack-protector-strong # stack protection
        --param=ssp-buffer-size=4 # stack protection
    )

    # TODO enable -fhardened in the future when it can work w/o warnings
    #check_cxx_compiler_flag("-fhardened" COMPILER_SUPPORTS_HARDENED)
    #if (COMPILER_SUPPORTS_HARDENED AND NOT ${TESTS_CLANGTIDY} AND NOT ${WITHOUT_PIE})
    #    list(APPEND ANDROMEDA_CXX_OPTS "-fhardened")
    #else()
        #message(NOTICE "Not using -fhardened (compiler support? PIE? clang-tidy?)")
        list(APPEND ANDROMEDA_CXX_OPTS
            # NOTE FORTIFY_SOURCE requires optimization
            $<$<NOT:$<CONFIG:Debug>>:-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3>

            # enable glibc debug mode and assertions
            -D_GLIBCXX_ASSERTIONS # c++stdlib assertions
            $<$<CONFIG:Debug>:-D_GLIBCXX_DEBUG>
        )
    #endif()

    if (NOT APPLE)
        set(ANDROMEDA_LINK_OPTS
            -Wl,-z,relro # relocation read-only
            -Wl,-z,now   # bind symbols at startup
            -Wl,-z,noexecstack # no-exec stack
        )
    endif()

    ### ADD PIC/PIE ###
    option(WITHOUT_PIE "Disable position independent executable" OFF)
    if (NOT ${WITHOUT_PIE})
        list(APPEND ANDROMEDA_CXX_OPTS -fPIE)
        list(APPEND ANDROMEDA_LINK_OPTS -Wl,-pie -pie)
    endif()

    ### ADD SANITIZERS ###
    if (APPLE)
        set(SANITIZE_DEFAULT "address,undefined")
    elseif (${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
        set(SANITIZE_DEFAULT "undefined")
    else()
        set(SANITIZE_DEFAULT "address,leak,undefined")
    endif()
    set(SANITIZE ${SANITIZE_DEFAULT} CACHE STRING "Build with sanitizers")
    if (NOT ${SANITIZE} STREQUAL "" AND NOT ${SANITIZE} STREQUAL "none")
        list(APPEND ANDROMEDA_CXX_OPTS $<$<CONFIG:Debug>:-fsanitize=${SANITIZE}>)
        list(APPEND ANDROMEDA_LINK_OPTS $<$<CONFIG:Debug>:-fsanitize=${SANITIZE}>)
    endif()

endif() # MSVC

function (andromeda_analyze target)
    if (${TESTS_CLANGTIDY})
        # clang-tidy rules are set in .clang-tidy
        # clang versions [10,16] should have no errors
        set(CLANG_TIDY_FLAGS "clang-tidy;--quiet")
        if (NOT ${ALLOW_WARNINGS})
            list(APPEND CLANG_TIDY_FLAGS "--warnings-as-errors=*")
        endif()
        set_target_properties(${target} PROPERTIES C_CLANG_TIDY "${CLANG_TIDY_FLAGS}")
        set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_FLAGS}")
    endif()

    #set_target_properties(${target} PROPERTIES(C_INCLUDE_WHAT_YOU_USE include-what-you-use)
    #set_target_properties(${target} PROPERTIES(CXX_INCLUDE_WHAT_YOU_USE include-what-you-use)
endfunction()

if (${TESTS_CPPCHECK})
    # CXX_CPPCHECK ${target} does not seem to work...
    set(CMAKE_CXX_CPPCHECK "cppcheck;--std=c++17;--quiet;--inline-suppr"
        "--enable=style,performance,portability"
        "--suppress=*:*_deps/*"
        "--suppress=*:*_autogen/*" # qt
        "--suppress=unmatchedSuppression"
        "--suppress=unknownMacro" # qt
        "--suppress=missingInclude"
        "--suppress=missingIncludeSystem"
        "--suppress=useStlAlgorithm" # annoying
        "--suppress=comparisonOfFuncReturningBoolError" # catch2
        "--suppress=assertWithSideEffecs" # annoying
        "--suppress=checkersReport"
        PARENT_SCOPE)
    # cppcheck is too buggy...
    #if (NOT ${ALLOW_WARNINGS})
    #    list(APPEND CPPCHECK_FLAGS "--error-exitcode=1")
    #endif()
endif()

function(andromeda_compile_opts myname)
    target_compile_options(${myname} PRIVATE ${ANDROMEDA_CXX_WARNS} ${ANDROMEDA_CXX_OPTS})
    target_compile_definitions(${myname} PRIVATE ${ANDROMEDA_CXX_DEFS})
endfunction()

function(andromeda_link_opts myname)
    target_link_options(${myname} PRIVATE ${ANDROMEDA_LINK_OPTS})
endfunction()

function(andromeda_lib lib_name sources)
    add_library(${lib_name} STATIC)
    target_sources(${lib_name} PRIVATE ${sources})
    set_target_properties(${lib_name} PROPERTIES PREFIX "")
    andromeda_analyze(${lib_name})
    andromeda_compile_opts(${lib_name})
endfunction()

function(andromeda_bin bin_name sources)
    add_executable(${bin_name})
    target_sources(${bin_name} PRIVATE ${sources})
    andromeda_analyze(${bin_name})
    andromeda_compile_opts(${bin_name})
    andromeda_link_opts(${bin_name})
endfunction()

# if a unit test binary runs and fails, it will be deleted
# turn this option off to keep it around for debugging!
option(TESTS_CATCH2_RUN "Run built unit tests" ON)

function (andromeda_test test_name)
    target_link_libraries(${test_name} PRIVATE 
        Catch2::Catch2WithMain trompeloeil::trompeloeil)
    target_compile_options(Catch2 PRIVATE ${ANDROMEDA_CXX_OPTS})
    target_compile_options(Catch2WithMain PRIVATE ${ANDROMEDA_CXX_OPTS})
    if (${TESTS_CATCH2_RUN})
        add_custom_command(TARGET ${test_name} POST_BUILD COMMAND ${test_name})
    endif()
endfunction()
