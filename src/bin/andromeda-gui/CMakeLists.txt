cmake_minimum_required(VERSION 3.16)
project(andromeda-gui VERSION 1.0.0)

include(../../../andromeda.cmake)

# set up required Qt packages

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

# build the andromeda-gui executable

set(SOURCE_FILES 
    main.cpp
    BackendManager.cpp
    MountManager.cpp
    )

qt_add_executable(andromeda-gui ${SOURCE_FILES})

target_compile_options(andromeda-gui PRIVATE ${ANDROMEDA_WARNINGS})
target_include_directories(andromeda-gui
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(andromeda-gui PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

install(TARGETS andromeda-gui
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET andromeda-gui
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

if (WIN32)
    install(TARGETS andromeda-gui RUNTIME_DEPENDENCIES
      PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "^qt.*\\.dll"
      POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
      DIRECTORIES $ENV{PATH}
    )
endif()

add_subdirectory(windows)

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
