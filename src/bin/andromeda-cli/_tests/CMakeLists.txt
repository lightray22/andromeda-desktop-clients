
include(../../../andromeda.cmake)

add_executable(andromeda-cli_tests)

set(SOURCE_FILES 
    CommandLineTest.cpp
    )

target_sources(andromeda-cli_tests PRIVATE ${SOURCE_FILES})
target_include_directories(andromeda-cli_tests
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

andromeda_bin(andromeda-cli_tests)
andromeda_test(andromeda-cli_tests)

target_link_libraries(andromeda-cli_tests PRIVATE libandromeda)
