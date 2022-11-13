cmake_minimum_required(VERSION 3.16)
project(andromeda-gui VERSION 1.0.0)

include(../../../andromeda.cmake)

# set up required Qt packages

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# build the andromeda-gui executable

set(SOURCE_FILES main.cpp 
    MainWindow.hpp MainWindow.cpp MainWindow.ui)

qt_add_executable(andromeda-gui ${SOURCE_FILES})

target_compile_options(andromeda-gui PRIVATE ${ANDROMEDA_WARNINGS})

# include/link libandromeda

if (NOT TARGET andromeda-fuse)
    add_subdirectory(../../lib/andromeda-fuse lib/andromeda-fuse)
endif()

if (NOT TARGET andromeda-sync)
    add_subdirectory(../../lib/andromeda-sync lib/andromeda-sync)
endif()

target_link_libraries(andromeda-gui PRIVATE andromeda-fuse)
#target_link_libraries(andromeda-gui PRIVATE andromeda-sync)

# include/link Qt libraries

target_link_libraries(andromeda-gui PRIVATE Qt6::Widgets)
