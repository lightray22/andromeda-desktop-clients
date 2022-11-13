#ifndef A2GUI_MAINWINDOW_H
#define A2GUI_MAINWINDOW_H

#include <memory>
#include <thread>

#include <QMainWindow>

#include "andromeda/Utilities.hpp"

namespace Ui { class MainWindow; }

/** The main Andromeda GUI window */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow();

    virtual ~MainWindow();

public slots:

    void Mount();
    void Unmount();

private:

    std::unique_ptr<std::thread> mFuseThread;

    std::unique_ptr<Ui::MainWindow> mUi;

    Andromeda::Debug mDebug;
};

#endif // MAINWINDOW_H
