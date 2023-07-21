
set(SOURCE_FILES 
    AccountTab.hpp
    AccountTab.cpp
    AccountTab.ui
    LoginDialog.hpp
    LoginDialog.cpp
    LoginDialog.ui
    MainWindow.hpp
    MainWindow.cpp
    MainWindow.ui
    MountsDialog.hpp
    MountsDialog.cpp
    MountsDialog.ui
    SystemTray.hpp
    SystemTray.cpp
    )

target_sources(andromeda-gui PRIVATE ${SOURCE_FILES})

target_sources(andromeda-gui PRIVATE resources/resources.qrc)

# https://doc.qt.io/qt-6/appicon.html
if (WIN32)
    set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/resources/andromeda-gui.rc")
    target_sources(andromeda-gui PRIVATE ${app_icon_resource_windows})
elseif (APPLE)
    set(MACOSX_BUNDLE_ICON_FILE andromeda-gui.icns)
    set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/resources/andromeda-gui.icns")
    set_source_files_properties(${app_icon_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(andromeda-gui PRIVATE ${app_icon_macos})
endif()
