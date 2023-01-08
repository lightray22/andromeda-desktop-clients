
#include <QtGui/QIcon>
#include <QtGui/QPixmap>

#include "SystemTray.hpp"
#include "MainWindow.hpp"

/*****************************************************/
SystemTray::SystemTray(QApplication& application, MainWindow& mainWindow) :
    QSystemTrayIcon(QIcon(QPixmap(32,32))),
    mActionShow("Show"),
    mActionExit("Exit"),
    mApplication(application),
    mMainWindow(mainWindow),
    mDebug("SystemTray",this)
{
    MDBG_INFO("()");

    mContextMenu.addAction(&mActionShow);
    mContextMenu.addAction(&mActionExit);

    QObject::connect(&mActionShow, &QAction::triggered, &mMainWindow, &MainWindow::show);
    QObject::connect(&mActionExit, &QAction::triggered, &mApplication, &QApplication::quit);

    QObject::connect(this, &SystemTray::activated, [&](QSystemTrayIcon::ActivationReason reason){
        if (reason == QSystemTrayIcon::DoubleClick) mMainWindow.show();
    });

    setContextMenu(&mContextMenu);
    setToolTip("Andromeda");
}

/*****************************************************/
SystemTray::~SystemTray()
{
    MDBG_INFO("()");
}
