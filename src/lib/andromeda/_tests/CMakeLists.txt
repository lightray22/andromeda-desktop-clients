
add_executable(libandromeda_tests)

set(SOURCE_FILES 
    BaseOptionsTest.cpp
    UtilitiesTest.cpp
    )

target_sources(libandromeda_tests PRIVATE ${SOURCE_FILES})
target_include_directories(libandromeda_tests
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_options(libandromeda_tests PRIVATE ${ANDROMEDA_WARNINGS})
target_link_libraries(libandromeda_tests PRIVATE libandromeda)
target_link_libraries(libandromeda_tests PRIVATE Catch2::Catch2WithMain)

# Run the test - if it fails and you don't want it deleted, comment this line
add_custom_command(TARGET libandromeda_tests POST_BUILD COMMAND libandromeda_tests)
