
#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "AccountTab.hpp"
#include "LoginDialog.hpp"

#include "andromeda-gui/BackendManager.hpp"
#include "andromeda-gui/MountManager.hpp"

#include "andromeda/Backend.hpp"
using Andromeda::Backend;

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
    mDebug << __func__ << "()"; mDebug.Info();

    QMainWindow::show(); // base class
    activateWindow(); // bring to front

    if (!mBackendManager.HasBackend()) AddAccount();
}

/*****************************************************/
void MainWindow::closeEvent(QCloseEvent* event)
{
    mDebug << __func__ << "()"; mDebug.Info();

    if (!event->spontaneous() || !mBackendManager.HasBackend())
    {
        mDebug.Info("closing MainWindow");
        QMainWindow::closeEvent(event);
    }
    else
    {
        mDebug.Info("hiding MainWindow");
        event->ignore(); hide();
    }
}

/*****************************************************/
void MainWindow::AddAccount()
{
    mDebug << __func__ << "()"; mDebug.Info();

    // TODO where/how to prevent adding duplicate accounts?
    // TODO disable mount/unmount/browse/remove account menu buttons until valid

    LoginDialog loginDialog(*this, mBackendManager);
    if (loginDialog.exec())
    {
        Backend* backend { loginDialog.GetBackend() };
        if (backend != nullptr)
        {
            AccountTab* accountTab { new AccountTab(*this, mMountManager, *backend) };
            mQtUi->tabAccounts->addTab(accountTab, backend->GetName(true).c_str());
        }
    }
}

/*****************************************************/
void MainWindow::RemoveAccount()
{
    mDebug << __func__ << "()"; mDebug.Info();

    // TODO should have a confirmation prompt for this

    AccountTab* accountTab { GetCurrentTab() };
    if (accountTab != nullptr)
    {
        Backend& tabBackend { accountTab->GetBackend() };

        int tabIndex { mQtUi->tabAccounts->currentIndex() };
        mQtUi->tabAccounts->removeTab(tabIndex); delete accountTab;

        mBackendManager.RemoveBackend(tabBackend);
    }
}

/*****************************************************/
AccountTab* MainWindow::GetCurrentTab()
{
    QWidget* widgetTab { mQtUi->tabAccounts->currentWidget() };
    return (widgetTab != nullptr) ? dynamic_cast<AccountTab*>(widgetTab) : nullptr;
}

/*****************************************************/
void MainWindow::MountCurrent()
{
    mDebug << __func__ << "()"; mDebug.Info();

    AccountTab* accountTab { GetCurrentTab() };
    if (accountTab != nullptr) accountTab->Mount();
}

/*****************************************************/
void MainWindow::UnmountCurrent()
{
    mDebug << __func__ << "()"; mDebug.Info();

    AccountTab* accountTab { GetCurrentTab() };
    if (accountTab != nullptr) accountTab->Unmount();
}

/*****************************************************/
void MainWindow::BrowseCurrent()
{
    mDebug << __func__ << "()"; mDebug.Info();

    AccountTab* accountTab { GetCurrentTab() };
    if (accountTab != nullptr) accountTab->Browse();
}
