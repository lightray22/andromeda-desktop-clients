cmake_minimum_required(VERSION 3.16)

# build the andromeda-sync library

add_library(andromeda-sync)

#set(SOURCE_FILES FuseAdapter.cpp)

target_sources(andromeda-sync PRIVATE ${SOURCE_FILES})
target_include_directories(andromeda-sync
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(andromeda-sync PRIVATE ${ANDROMEDA_COMPILE_OPTS})

# include/link libandromeda

target_link_libraries(andromeda-sync PRIVATE andromeda)
