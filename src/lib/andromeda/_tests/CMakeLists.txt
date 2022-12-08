
add_executable(andromeda_tests)

set(SOURCE_FILES 
    BaseOptionsTest.cpp
    HTTPRunnerTest.cpp
    UtilitiesTest.cpp
    )

target_sources(andromeda_tests PRIVATE ${SOURCE_FILES})
target_include_directories(andromeda_tests
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_options(andromeda_tests PRIVATE ${ANDROMEDA_WARNINGS})
target_link_libraries(andromeda_tests PRIVATE andromeda)
target_link_libraries(andromeda_tests PRIVATE Catch2::Catch2WithMain)

# Run the test - if it fails and you don't want it deleted, comment this line
add_custom_command(TARGET andromeda_tests POST_BUILD COMMAND andromeda_tests)
