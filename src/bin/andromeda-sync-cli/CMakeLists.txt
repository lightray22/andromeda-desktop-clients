cmake_minimum_required(VERSION 3.16)
project(andromeda-sync-cli VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# build the andromeda-sync-cli executable

#set(SOURCE_FILES main.cpp Options.cpp)
#add_executable(andromeda-sync-cli ${SOURCE_FILES})

#target_compile_options(andromeda-sync-cli PRIVATE -Wall -Wextra -Werror
#    -Wno-unused-parameter -pedantic -pedantic-errors)

# include/link libandromeda-sync

#if (NOT TARGET andromeda-sync)
#    add_subdirectory(../../lib/andromeda-sync lib/andromeda-sync)
#endif()

#if (NOT TARGET andromeda)
#    add_subdirectory(../../lib/andromeda lib/andromeda)
#endif()

#target_link_libraries(andromeda-sync-cli PRIVATE andromeda-sync)
#target_link_libraries(andromeda-sync-cli PRIVATE andromeda)
