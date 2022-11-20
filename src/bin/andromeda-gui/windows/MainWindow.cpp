
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QMessageBox>

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "LoginDialog.hpp"

#include "andromeda-gui/BackendManager.hpp"
#include "andromeda-gui/MountManager.hpp"

#include "andromeda/Backend.hpp"
using Andromeda::Backend;
#include "andromeda-fuse/FuseAdapter.hpp"
using AndromedaFuse::FuseAdapter;

/*****************************************************/
MainWindow::MainWindow(BackendManager& backendManager, MountManager& mountManager) : QMainWindow(),
    mBackendManager(backendManager),
    mMountManager(mountManager),
    mQtUi(std::make_unique<Ui::MainWindow>()),
    mDebug("MainWindow")
{
    mDebug << __func__ << "()"; mDebug.Info();

    mQtUi->setupUi(this);
}

/*****************************************************/
MainWindow::~MainWindow()
{
    mDebug << __func__ << "()"; mDebug.Info();
}

/*****************************************************/
void MainWindow::show()
{
    QWidget::show(); // base class

    if (!mBackendManager.HasBackend()) AddAccount();
}

/*****************************************************/
void MainWindow::AddAccount()
{
    mDebug << __func__ << "()"; mDebug.Info();

    LoginDialog loginDialog(*this, mBackendManager);

    if (loginDialog.exec())
        mBackend = loginDialog.GetBackend();
}

/*****************************************************/
void MainWindow::RemoveAccount()
{
    mDebug << __func__ << "()"; mDebug.Info(); // TODO implement me
}

/*****************************************************/
void MainWindow::Mount(bool autoMount)
{
    mDebug << __func__ << "()"; mDebug.Info();

    if (!mBackend) return;

    FuseAdapter::Options fuseOptions;

    try
    {
        mMountManager.CreateMount(*mBackend, fuseOptions);
    }
    catch (const FuseAdapter::Exception& ex)
    {
        std::cout << ex.what() << std::endl; 

        QMessageBox errorBox(QMessageBox::Critical, QString("Mount Error"), ex.what()); 

        errorBox.exec(); return;
    }
    
    if (!autoMount) Browse();

    mQtUi->buttonMount->setEnabled(false);
    mQtUi->buttonUnmount->setEnabled(true);
    mQtUi->buttonBrowse->setEnabled(true);
}

/*****************************************************/
void MainWindow::Unmount()
{
    mDebug << __func__ << "()"; mDebug.Info();

    mMountManager.RemoveMount(); // TODO args?

    mQtUi->buttonMount->setEnabled(true);
    mQtUi->buttonUnmount->setEnabled(false);
    mQtUi->buttonBrowse->setEnabled(false);
}

/*****************************************************/
void MainWindow::Browse()
{
    std::string homeRoot { mMountManager.GetHomeRoot() };

    if (homeRoot.empty())
         { mDebug << __func__ << "... ERROR empty homeRoot!"; mDebug.Error(); return; } // TODO GUI error - also GetHomeRoot should throw exception if not found
    else { mDebug << __func__ << "(homeRoot: " << homeRoot << ")"; mDebug.Info(); }

    homeRoot.insert(0, "file:///");
    QDesktopServices::openUrl(QUrl(homeRoot.c_str()));
}
