cmake_minimum_required(VERSION 3.16)
project(libandromeda VERSION 1.0.0)

include(../../andromeda.cmake)

# build the Andromeda library

set(SOURCE_FILES 
    BaseOptions.cpp
    ConfigOptions.cpp
    Debug.cpp
    Utilities.cpp
    )
andromeda_lib(libandromeda "${SOURCE_FILES}")

target_include_directories(libandromeda
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

install(TARGETS libandromeda
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_subdirectory(backend)
add_subdirectory(database)
add_subdirectory(filesystem)

include(3rdparty.cmake)

# include/link libsqlite3

if (WIN32 AND DEFINED ENV{SQLITE3_ROOT_DIR})
    set(CMAKE_PREFIX_PATH $ENV{SQLITE3_ROOT_DIR})
endif()

include(FindSQLite3)
find_package(SQLite3 REQUIRED)

target_include_directories(libandromeda PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(libandromeda INTERFACE ${SQLite3_LIBRARIES})
