cmake_minimum_required(VERSION 3.16)
project(andromeda-sync VERSION 1.0.0)

include(../../andromeda.cmake)

# build the andromeda-sync executable

#set(SOURCE_FILES main.cpp Options.cpp)
#andromeda_bin(andromeda-sync "${SOURCE_FILES}"")

# include/link libandromeda-sync

#if (NOT TARGET libandromeda-sync)
#    add_subdirectory(../../lib/andromeda-sync lib/andromeda-sync)
#endif()

#target_link_libraries(andromeda-sync PRIVATE libandromeda-sync)
