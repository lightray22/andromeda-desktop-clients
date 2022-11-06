
add_executable(andromeda_tests)

set(SOURCE_FILES 
    demotest.cpp
    )

target_sources(andromeda_tests PRIVATE ${SOURCE_FILES})
target_include_directories(andromeda_tests
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

target_compile_options(andromeda_tests PRIVATE ${ANDROMEDA_WARNINGS})
target_link_libraries(andromeda_tests PRIVATE Catch2::Catch2WithMain)
