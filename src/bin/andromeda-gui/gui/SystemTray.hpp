#ifndef A2GUI_SYSTEMTRAY_H
#define A2GUI_SYSTEMTRAY_H

#if QTVER == 5
#include <QtWidgets/QAction>
#else // QTVER != 5
#include <QtGui/QAction>
#endif // QTVER

#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QSystemTrayIcon>

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"

namespace AndromedaGui {
namespace Gui {

class MainWindow;

/** The Andromeda system tray icon */
class SystemTray : public QSystemTrayIcon
{
    Q_OBJECT

public:

    SystemTray(QApplication& application, MainWindow& mainWindow);

    ~SystemTray() override;
    DELETE_COPY(SystemTray)
    DELETE_MOVE(SystemTray)

private:

    /** The context menu shown when right-clicking */
    QMenu mContextMenu;
    /** The menu option to show the main window */
    QAction mActionShow;
    /** The menu option to close the application */
    QAction mActionExit;

    QApplication& mApplication;
    MainWindow& mMainWindow;

    mutable Andromeda::Debug mDebug;
};

} // namespace Gui
} // namespace AndromedaGui

#endif // A2GUI_SYSTEMTRAY_H
