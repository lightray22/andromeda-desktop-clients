cmake_minimum_required(VERSION 3.16)

# build the andromeda-fuse-cli executable

set(SOURCE_FILES main.cpp Options.cpp)
add_executable(andromeda-fuse-cli ${SOURCE_FILES})

target_compile_options(andromeda-fuse-cli PRIVATE ${ANDROMEDA_COMPILE_OPTS})

# include/link libandromeda-fuse

target_link_libraries(andromeda-fuse-cli PRIVATE andromeda-fuse)
target_link_libraries(andromeda-fuse-cli PRIVATE andromeda)
