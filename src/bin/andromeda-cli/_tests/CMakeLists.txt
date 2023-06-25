
include(../../../andromeda.cmake)

set(SOURCE_FILES 
    CommandLineTest.cpp
    )

andromeda_bin(andromeda-cli_tests "${SOURCE_FILES}")
andromeda_test(andromeda-cli_tests)

target_include_directories(andromeda-cli_tests
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(andromeda-cli_tests PRIVATE libandromeda)
