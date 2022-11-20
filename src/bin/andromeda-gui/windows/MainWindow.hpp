#ifndef A2GUI_MAINWINDOW_H
#define A2GUI_MAINWINDOW_H

#include <memory>
#include <QtWidgets/QMainWindow>

#include "andromeda/Utilities.hpp"

class BackendManager;
class MountManager;
namespace Andromeda { class Backend; }
namespace Ui { class MainWindow; }

/** The main Andromeda GUI window */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(BackendManager& backendManager, MountManager& mountManager);

    virtual ~MainWindow();

public slots:

    void show(); // override

    void AddAccount();

    void RemoveAccount();

    void Mount(bool autoMount = false);

    void Unmount();

    void Browse();

private:

    BackendManager& mBackendManager;
    MountManager& mMountManager;

    Andromeda::Backend* mBackend { nullptr }; // TODO temp - have one per tab

    std::unique_ptr<Ui::MainWindow> mQtUi;

    Andromeda::Debug mDebug;
};

#endif // A2GUI_MAINWINDOW_H
