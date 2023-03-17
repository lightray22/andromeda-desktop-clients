cmake_minimum_required(VERSION 3.16)
project(libandromeda-sync VERSION 1.0.0)

include(../../../andromeda.cmake)

# build the andromeda-sync library

#add_library(libandromeda-sync STATIC)
#set_target_properties(libandromeda-sync PROPERTIES PREFIX "")

#set(SOURCE_FILES FuseAdapter.cpp)

#target_sources(libandromeda-sync PRIVATE ${SOURCE_FILES})
#target_include_directories(libandromeda-sync
#    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

#target_compile_options(libandromeda-sync PRIVATE ${ANDROMEDA_CXX_WARNS} ${ANDROMEDA_CXX_OPTS})
#target_compile_definitions(libandromeda-sync PRIVATE ${ANDROMEDA_CXX_DEFS})

# include/link libandromeda

#if (NOT TARGET libandromeda)
#    add_subdirectory(../andromeda lib/andromeda)
#endif()

#target_link_libraries(libandromeda-sync PUBLIC libandromeda)
