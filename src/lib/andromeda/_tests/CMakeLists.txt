
include(../../../andromeda.cmake)

add_executable(libandromeda_tests)

set(SOURCE_FILES 
    BaseOptionsTest.cpp
    OrderedMap.cpp
    UtilitiesTest.cpp
    )

option(MUTEX_TESTS "Build mutex tests" OFF)
# These have timing assumptions, not reliable
if (MUTEX_TESTS)
    list(APPEND SOURCE_FILES
        SemaphorTest.cpp
        SharedMutexTest.cpp
    )
endif()

target_sources(libandromeda_tests PRIVATE ${SOURCE_FILES})
target_include_directories(libandromeda_tests
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

andromeda_bin(libandromeda_tests)
andromeda_test(libandromeda_tests)

target_link_libraries(libandromeda_tests PRIVATE libandromeda)
