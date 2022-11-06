
add_executable(andromeda__tests)

set(SOURCE_FILES 
    UtilitiesTest.cpp
    )

target_sources(andromeda__tests PRIVATE ${SOURCE_FILES})
target_include_directories(andromeda__tests
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_options(andromeda__tests PRIVATE ${ANDROMEDA_WARNINGS})
target_link_libraries(andromeda__tests PRIVATE andromeda)
target_link_libraries(andromeda__tests PRIVATE Catch2::Catch2WithMain)
