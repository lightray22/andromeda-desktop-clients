cmake_minimum_required(VERSION 3.16)
project(andromeda-fuse VERSION 1.0.0)

include(../../../andromeda.cmake)

# build the andromeda-fuse executable

set(SOURCE_FILES main.cpp Options.cpp)
add_executable(andromeda-fuse ${SOURCE_FILES})

target_compile_options(andromeda-fuse PRIVATE ${ANDROMEDA_CXX_WARNS} ${ANDROMEDA_CXX_OPTS})
target_compile_definitions(andromeda-fuse PRIVATE ${ANDROMEDA_CXX_DEFS})
target_link_options(andromeda-fuse PRIVATE ${ANDROMEDA_LINK_OPTS})

install(TARGETS andromeda-fuse
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# include/link libandromeda-fuse

if (NOT TARGET libandromeda-fuse)
    add_subdirectory(../../lib/andromeda-fuse lib/andromeda-fuse)
endif()

target_link_libraries(andromeda-fuse PRIVATE libandromeda-fuse)
