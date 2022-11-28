
#include <QtWidgets/QMessageBox>

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "AccountTab.hpp"
#include "LoginDialog.hpp"

#include "andromeda-gui/BackendContext.hpp"

/*****************************************************/
MainWindow::MainWindow() : QMainWindow(),
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

    if (GetCurrentTab() == nullptr) AddAccount();
}

/*****************************************************/
void MainWindow::closeEvent(QCloseEvent* event)
{
    mDebug << __func__ << "()"; mDebug.Info();

    if (!event->spontaneous() || GetCurrentTab() == nullptr)
    {
        mDebug << __func__ << "... closing"; mDebug.Info();

        QMainWindow::closeEvent(event);
    }
    else
    {
        mDebug << __func__ << "... hiding"; mDebug.Info();

        event->ignore(); hide();
    }
}

/*****************************************************/
void MainWindow::AddAccount()
{
    mDebug << __func__ << "()"; mDebug.Info();

    // TODO where/how to prevent adding duplicate accounts?

    LoginDialog loginDialog(*this);
    if (loginDialog.exec())
    {
        AccountTab* accountTab { new AccountTab(*this, loginDialog.TakeBackend()) };

        mQtUi->tabAccounts->setCurrentIndex(
            mQtUi->tabAccounts->addTab(accountTab, accountTab->GetTabName().c_str()));

        // TODO this shouldn't all be always enabled, accountTab will need to manage
        mQtUi->actionMount_Storage->setEnabled(true);
        mQtUi->actionUnmount_Storage->setEnabled(true);
        mQtUi->actionBrowse_Storage->setEnabled(true);
        mQtUi->actionRemove_Account->setEnabled(true);
    }
}

/*****************************************************/
void MainWindow::RemoveAccount()
{
    mDebug << __func__ << "()"; mDebug.Info();

    if (QMessageBox::question(this, "Remove Account", "Are you sure?") == QMessageBox::Yes)
        { mDebug << __func__ << "... confirmed"; mDebug.Info(); }
    else return; // early return!

    AccountTab* accountTab { GetCurrentTab() };
    if (accountTab != nullptr)
    {
        int tabIndex { mQtUi->tabAccounts->currentIndex() };
        mQtUi->tabAccounts->removeTab(tabIndex); delete accountTab;
    }

    if (GetCurrentTab() == nullptr) // no accounts left
    {
        mQtUi->actionMount_Storage->setEnabled(false);
        mQtUi->actionUnmount_Storage->setEnabled(false);
        mQtUi->actionBrowse_Storage->setEnabled(false);
        mQtUi->actionRemove_Account->setEnabled(false);
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
