
add_executable(libandromeda_tests)

set(SOURCE_FILES 
    BaseOptionsTest.cpp
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

target_compile_options(libandromeda_tests PRIVATE ${ANDROMEDA_CXX_WARNS} ${ANDROMEDA_CXX_OPTS})
target_compile_definitions(libandromeda_tests PRIVATE ${ANDROMEDA_CXX_DEFS})
target_link_libraries(libandromeda_tests PRIVATE libandromeda Catch2::Catch2WithMain)

# Run the test - if it fails and you don't want it deleted, comment this line
add_custom_command(TARGET libandromeda_tests POST_BUILD COMMAND libandromeda_tests)
