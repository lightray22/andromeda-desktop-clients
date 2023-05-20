cmake_minimum_required(VERSION 3.16)
project(andromeda-cli VERSION 1.0.0)

include(../../andromeda.cmake)

# build the andromeda-cli executable

set(SOURCE_FILES main.cpp Options.cpp CommandLine.cpp)
add_executable(andromeda-cli ${SOURCE_FILES})
andromeda_bin(andromeda-cli)

install(TARGETS andromeda-cli
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# include/link libandromeda

if (NOT TARGET libandromeda)
    add_subdirectory(../../lib/andromeda lib/andromeda)
endif()

target_link_libraries(andromeda-cli PRIVATE libandromeda)
