cmake_minimum_required(VERSION 3.16)
project(andromeda-fuse-cli VERSION 1.0.0)

include(../../../andromeda.cmake)

# build the andromeda-fuse-cli executable

set(SOURCE_FILES main.cpp Options.cpp)
add_executable(andromeda-fuse-cli ${SOURCE_FILES})

target_compile_options(andromeda-fuse-cli PRIVATE ${ANDROMEDA_WARNINGS})

install(TARGETS andromeda-fuse-cli
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# include/link libandromeda-fuse

if (NOT TARGET andromeda-fuse)
    add_subdirectory(../../lib/andromeda-fuse lib/andromeda-fuse)
endif()

target_link_libraries(andromeda-fuse-cli PRIVATE andromeda-fuse)
