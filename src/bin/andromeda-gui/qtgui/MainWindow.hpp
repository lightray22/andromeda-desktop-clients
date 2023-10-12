#ifndef A2GUI_MAINWINDOW_H
#define A2GUI_MAINWINDOW_H

#include <memory>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QMainWindow>

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"

namespace Ui { class MainWindow; }

namespace Andromeda { 
    namespace Backend { class SessionStore; }
    namespace Database { class ObjectDatabase; }
    namespace Filesystem { namespace Filedata { class CacheManager; } }
}

namespace AndromedaGui {
class BackendContext;

namespace QtGui {
class AccountTab;
class DebugWindow;

/** The main Andromeda GUI window */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    /** Instantiates the main window UI with resources */
    explicit MainWindow(
        Andromeda::Filesystem::Filedata::CacheManager* cacheManager,
        Andromeda::Database::ObjectDatabase* objDatabase);

    ~MainWindow() override;
    DELETE_COPY(MainWindow)
    DELETE_MOVE(MainWindow)

    void closeEvent(QCloseEvent* event) override;

    /** Add an account tab for an existing session, shows an error box on failure */
    void TryLoadAccount(Andromeda::Backend::SessionStore& session);

public slots:

    /** Calls fullShow() on this window */
    void fullShow();

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

    /** Show the Debug Log window */
    void ShowDebug();

private:

    /** Returns the current AccountTab or nullptr if none */
    AccountTab* GetCurrentTab();

    /** Adds a new account tab for a created backend context */
    void AddAccountTab(std::unique_ptr<BackendContext> backendCtx);

    /** Removes the account tab at the given index */
    void RemoveAccountTab(int tabIndex);

    /** Removes the account tab at the given pointer */
    void RemoveAccountTab(AccountTab* accountTab);

    mutable Andromeda::Debug mDebug;

    /** Global cache manager to apply to all mounts (maybe null!) */
    Andromeda::Filesystem::Filedata::CacheManager* mCacheManager;

    /** ObjectDatabase instance for object storage (maybe null!) */
    Andromeda::Database::ObjectDatabase* mObjDatabase;

    std::unique_ptr<Ui::MainWindow> mQtUi;
    std::unique_ptr<DebugWindow> mDebugWindow;
};

} // namespace QtGui
} // namespace AndromedaGui

#endif // A2GUI_MAINWINDOW_H
