#ifndef A2GUI_SYSTEMTRAY_H
#define A2GUI_SYSTEMTRAY_H

#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QSystemTrayIcon>

#include "andromeda/Utilities.hpp"

class MainWindow;

/** The Andromeda system tray icon */
class SystemTray : public QSystemTrayIcon
{
    Q_OBJECT

public:

    SystemTray(QApplication& application, MainWindow& mainWindow);

    virtual ~SystemTray();

private:

    QMenu mContextMenu;
    QAction mActionShow;
    QAction mActionExit;

    QApplication& mApplication;
    MainWindow& mMainWindow;

    Andromeda::Debug mDebug;
};

#endif // A2GUI_SYSTEMTRAY_H
