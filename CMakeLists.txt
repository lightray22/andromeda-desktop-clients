cmake_minimum_required(VERSION 3.18)
project(andromeda-desktop)

add_subdirectory(src/bin/andromeda-fuse-cli cli-fuse)
add_subdirectory(src/bin/andromeda-sync-cli cli-sync)
add_subdirectory(src/bin/andromeda-gui gui)
