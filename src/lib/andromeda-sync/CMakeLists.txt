cmake_minimum_required(VERSION 3.16)
project(libandromeda-sync VERSION 1.0.0)

include(../../../CMakeLists.inc.txt)

# build the andromeda-sync library

#add_library(andromeda-sync)

#set(SOURCE_FILES FuseAdapter.cpp)

#target_sources(andromeda-sync PRIVATE ${SOURCE_FILES})
#target_include_directories(andromeda-sync
#    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

#target_compile_options(andromeda-sync PRIVATE ${ANDROMEDA_WARNINGS})

# include/link libandromeda

#if (NOT TARGET andromeda)
#    add_subdirectory(../andromeda lib/andromeda)
#endif()

#target_link_libraries(andromeda-sync PUBLIC andromeda)
