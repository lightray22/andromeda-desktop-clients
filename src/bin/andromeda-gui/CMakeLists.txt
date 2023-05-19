cmake_minimum_required(VERSION 3.16)
project(andromeda-gui VERSION 1.0.0)

include(../../andromeda.cmake)

# set up required Qt packages

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets)

if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_standard_project_setup()
else()
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
endif()

# build the andromeda-gui executable

set(SOURCE_FILES 
    main.cpp Options.cpp
    BackendContext.cpp
    MountContext.cpp
    )

if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(andromeda-gui ${SOURCE_FILES})
else()
    add_executable(andromeda-gui ${SOURCE_FILES})
endif()
    
target_compile_options(andromeda-gui PRIVATE ${ANDROMEDA_CXX_WARNS} ${ANDROMEDA_CXX_OPTS})
target_compile_definitions(andromeda-gui PRIVATE ${ANDROMEDA_CXX_DEFS} QTVER=${QT_VERSION_MAJOR})
target_link_options(andromeda-gui PRIVATE ${ANDROMEDA_LINK_OPTS})

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

if (WIN32 OR APPLE)
    qt_generate_deploy_app_script(
        TARGET andromeda-gui
        OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS
    )
    install(SCRIPT ${deploy_script})
endif()
if (WIN32)
    install(TARGETS andromeda-gui RUNTIME_DEPENDENCIES
      PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "^qt.*\\.dll"
      POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
      DIRECTORIES $ENV{PATH}
    )
    # don't need this, not sure why it installs them
    install(CODE "file(REMOVE ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/D3Dcompiler_47.dll)")
    install(CODE "file(REMOVE ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/opengl32sw.dll)")
endif()

add_subdirectory(gui)

# include/link libandromeda

if (NOT TARGET libandromeda-fuse)
    add_subdirectory(../../lib/andromeda-fuse lib/andromeda-fuse)
endif()

if (NOT TARGET libandromeda-sync)
    add_subdirectory(../../lib/andromeda-sync lib/andromeda-sync)
endif()

target_link_libraries(andromeda-gui PRIVATE libandromeda-fuse)
#target_link_libraries(andromeda-gui PRIVATE libandromeda-sync)

# include/link Qt libraries

target_link_libraries(andromeda-gui PRIVATE
    Qt${QT_VERSION_MAJOR}::Core 
    Qt${QT_VERSION_MAJOR}::Gui 
    Qt${QT_VERSION_MAJOR}::Widgets)
