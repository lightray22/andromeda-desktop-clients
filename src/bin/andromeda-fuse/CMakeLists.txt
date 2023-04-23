cmake_minimum_required(VERSION 3.16)
project(andromeda-fuse VERSION 1.0.0)

include(../../../andromeda.cmake)

# build the andromeda-fuse executables

set(SOURCE_FILES main.cpp Options.cpp)
add_executable(andromeda-fuse ${SOURCE_FILES})

install(TARGETS andromeda-fuse
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (NOT WIN32)
    add_executable(mount.andromeda mount.cpp)
endif()

install(TARGETS mount.andromeda
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
)

foreach(EXECUTABLE andromeda-fuse mount.andromeda)

    target_compile_options(${EXECUTABLE} PRIVATE ${ANDROMEDA_CXX_WARNS} ${ANDROMEDA_CXX_OPTS})
    target_compile_definitions(${EXECUTABLE} PRIVATE ${ANDROMEDA_CXX_DEFS})
    target_link_options(${EXECUTABLE} PRIVATE ${ANDROMEDA_LINK_OPTS})

    # include/link libandromeda-fuse

    if (NOT TARGET libandromeda-fuse)
        add_subdirectory(../../lib/andromeda-fuse lib/andromeda-fuse)
    endif()

    target_link_libraries(${EXECUTABLE} PRIVATE libandromeda-fuse)

endforeach(EXECUTABLE)
