#ifndef A2GUI_MAINWINDOW_H
#define A2GUI_MAINWINDOW_H

#include <memory>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QMainWindow>

#include "andromeda/Debug.hpp"
#include "andromeda/filesystem/filedata/CacheManager.hpp"

namespace AndromedaGui {
namespace Gui {

class AccountTab;

namespace Ui { class MainWindow; }

/** The main Andromeda GUI window */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow();

    virtual ~MainWindow();

    void closeEvent(QCloseEvent* event) override;

public slots:

    void show(); // override

    /** GUI action to add a new account */
    void AddAccount();

    /** GUI action to remove the current account */
    void RemoveAccount();

    /** GUI action to mount the current account's files */
    void MountCurrent();

    /** GUI action to unmount the current account's files */
    void UnmountCurrent();

    /** GUI action to browse the current account's files */
    void BrowseCurrent();

    /** Show the About popup window */
    void ShowAbout();

private:

    /** Returns the current AccountTab or nullptr if none */
    AccountTab* GetCurrentTab();

    std::unique_ptr<Ui::MainWindow> mQtUi;

    /** Global cache manager to apply to all mounts */
    Andromeda::Filesystem::Filedata::CacheManager mCacheManager;

    Andromeda::Debug mDebug;
};

} // namespace Gui
} // namespace AndromedaGui

#endif // A2GUI_MAINWINDOW_H
