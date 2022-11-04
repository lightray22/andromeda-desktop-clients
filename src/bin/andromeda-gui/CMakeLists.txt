cmake_minimum_required(VERSION 3.16)
project(andromeda-gui VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# build the andromeda-gui executable

#set(SOURCE_FILES main.cpp Options.cpp)
#add_executable(andromeda-gui ${SOURCE_FILES})

#target_compile_options(andromeda-fuse-cli PRIVATE -Wall -Wextra -Werror
#    -Wno-unused-parameter -pedantic -pedantic-errors)

# include/link libandromeda

#if (NOT TARGET andromeda-fuse)
#    add_subdirectory(../../lib/andromeda-fuse lib/andromeda-fuse)
#endif()

#if (NOT TARGET andromeda-sync)
#    add_subdirectory(../../lib/andromeda-sync lib/andromeda-sync)
#endif()

#if (NOT TARGET andromeda)
#    add_subdirectory(../../lib/andromeda lib/andromeda)
#endif()

#target_link_libraries(andromeda-gui PRIVATE andromeda-fuse)
#target_link_libraries(andromeda-gui PRIVATE andromeda-sync)
#target_link_libraries(andromeda-gui PRIVATE andromeda)
