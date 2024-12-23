cmake_minimum_required(VERSION 3.22)
project(andromeda-desktop-clients VERSION 1.0.0)

add_subdirectory(src/lib/andromeda lib/andromeda)
add_subdirectory(src/lib/andromeda-fuse lib/fuse)
add_subdirectory(src/lib/andromeda-sync lib/sync)

add_subdirectory(src/bin/andromeda-cli bin/cli)
add_subdirectory(src/bin/andromeda-fuse bin/fuse)
add_subdirectory(src/bin/andromeda-sync bin/sync)

option(WITHOUT_GUI "Don't build the Qt GUI" OFF)

if (NOT WITHOUT_GUI)
    add_subdirectory(src/bin/andromeda-gui bin/gui)
endif()
