cmake_minimum_required(VERSION 3.16)
project(andromeda-fuse VERSION 1.0.0)

include(../../andromeda.cmake)

# build the andromeda-fuse executables

set(SOURCE_FILES main.cpp Options.cpp)
add_executable(andromeda-fuse ${SOURCE_FILES})
set(EXECUTABLES andromeda-fuse)

target_compile_definitions(libandromeda-fuse PRIVATE _FILE_OFFSET_BITS=64)

install(TARGETS andromeda-fuse
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (NOT WIN32)
    add_executable(mount.andromeda mount.cpp)
    list(APPEND EXECUTABLES mount.andromeda)
    
    install(TARGETS mount.andromeda
        BUNDLE  DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
    )
endif()

foreach(EXECUTABLE ${EXECUTABLES})

    andromeda_bin(${EXECUTABLE})

    # include/link libandromeda-fuse

    if (NOT TARGET libandromeda-fuse)
        add_subdirectory(../../lib/andromeda-fuse lib/andromeda-fuse)
    endif()

    target_link_libraries(${EXECUTABLE} PRIVATE libandromeda-fuse)

endforeach(EXECUTABLE)
