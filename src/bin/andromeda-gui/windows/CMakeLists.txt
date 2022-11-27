
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
    )

target_sources(andromeda-gui PRIVATE ${SOURCE_FILES})
