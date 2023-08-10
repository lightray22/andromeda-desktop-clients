
set(SOURCE_FILES 
    AccountTab.cpp
    AccountTab.ui
    LoginDialog.cpp
    LoginDialog.ui
    MainWindow.cpp
    MainWindow.ui
    MountsDialog.cpp
    MountsDialog.ui
    SingleInstance.cpp
    SystemTray.cpp
    Utilities.cpp
    )

target_sources(andromeda-gui PRIVATE ${SOURCE_FILES})

target_sources(andromeda-gui PRIVATE resources/resources.qrc)
