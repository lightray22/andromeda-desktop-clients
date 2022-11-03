cmake_minimum_required(VERSION 3.16)
project(andromeda-desktop-clients VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(ANDROMEDA_COMPILE_OPTS -Wall -Wextra -Wno-unused-parameter -pedantic -pedantic-errors)

add_subdirectory(src/lib/andromeda lib/andromeda)
add_subdirectory(src/lib/andromeda-fuse lib/fuse)
add_subdirectory(src/lib/andromeda-sync lib/sync)

add_subdirectory(src/bin/andromeda-fuse-cli bin/cli-fuse)
add_subdirectory(src/bin/andromeda-sync-cli bin/cli-sync)
add_subdirectory(src/bin/andromeda-gui bin/gui)
