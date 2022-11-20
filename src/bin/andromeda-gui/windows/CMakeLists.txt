
set(SOURCE_FILES 
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
