
add_executable(andromeda-cli_tests)

set(SOURCE_FILES 
    CommandLineTest.cpp
    )

target_sources(andromeda-cli_tests PRIVATE ${SOURCE_FILES})
target_include_directories(andromeda-cli_tests
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_options(andromeda-cli_tests PRIVATE ${ANDROMEDA_CXX_WARNS} ${ANDROMEDA_CXX_OPTS})
target_compile_definitions(andromeda-cli_tests PRIVATE ${ANDROMEDA_CXX_DEFS})
target_link_options(andromeda-cli_tests PRIVATE ${ANDROMEDA_LINK_OPTS})

target_link_libraries(andromeda-cli_tests PRIVATE libandromeda)
target_link_libraries(andromeda-cli_tests PRIVATE Catch2::Catch2WithMain)

# Run the test - if it fails and you don't want it deleted, comment this line
add_custom_command(TARGET andromeda-cli_tests POST_BUILD COMMAND andromeda-cli_tests)
