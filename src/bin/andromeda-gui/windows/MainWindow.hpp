#ifndef A2GUI_MAINWINDOW_H
#define A2GUI_MAINWINDOW_H

#include <memory>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QMainWindow>

#include "andromeda/Utilities.hpp"

class AccountTab;

class BackendManager;
class MountManager;
namespace Ui { class MainWindow; }

/** The main Andromeda GUI window */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(BackendManager& backendManager, MountManager& mountManager);

    virtual ~MainWindow();

    void closeEvent(QCloseEvent* event) override;

public slots:

    void show(); // override

    void AddAccount();

    void RemoveAccount();

    void MountCurrent();

    void UnmountCurrent();

    void BrowseCurrent();

private:

    AccountTab* GetCurrentTab();

    BackendManager& mBackendManager;
    MountManager& mMountManager;

    std::unique_ptr<Ui::MainWindow> mQtUi;

    Andromeda::Debug mDebug;
};

#endif // A2GUI_MAINWINDOW_H
