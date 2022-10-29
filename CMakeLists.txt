cmake_minimum_required(VERSION 3.16)
project(andromeda-desktop)

add_subdirectory(src/lib/andromeda lib/andromeda)
add_subdirectory(src/lib/andromeda-fuse lib/fuse)
add_subdirectory(src/lib/andromeda-sync lib/sync)

add_subdirectory(src/bin/andromeda-fuse-cli bin/cli-fuse)
add_subdirectory(src/bin/andromeda-sync-cli bin/cli-sync)
add_subdirectory(src/bin/andromeda-gui bin/gui)
