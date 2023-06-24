
include(../../../andromeda.cmake)

set(SOURCE_FILES 
    base64Test.cpp
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

andromeda_bin(libandromeda_tests "${SOURCE_FILES}")
andromeda_test(libandromeda_tests)

target_include_directories(libandromeda_tests
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(libandromeda_tests PRIVATE libandromeda)
