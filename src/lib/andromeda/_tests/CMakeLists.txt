
include(../../../andromeda.cmake)

set(SOURCE_FILES 
    base64Test.cpp
    BaseOptionsTest.cpp
    CryptoTest.cpp
    OrderedMapTest.cpp
    SecureBufferTest.cpp
    StringUtilTest.cpp
    )

option(TESTS_MUTEX "Build mutex tests" OFF)
# These have timing assumptions, not reliable
if (TESTS_MUTEX)
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

add_subdirectory(backend)
add_subdirectory(database)
