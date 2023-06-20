cmake_minimum_required(VERSION 3.16)
project(libandromeda-sync VERSION 1.0.0)

include(../../andromeda.cmake)

# build the andromeda-sync library

#set(SOURCE_FILES FuseAdapter.cpp)
#andromeda_lib(libandromeda-sync "${SOURCE_FILES}")

#target_include_directories(libandromeda-sync
#    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

#install(TARGETS libandromeda-sync
#    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#)

# include/link libandromeda

#if (NOT TARGET libandromeda)
#    add_subdirectory(../andromeda lib/andromeda)
#endif()

#target_link_libraries(libandromeda-sync PUBLIC libandromeda)
