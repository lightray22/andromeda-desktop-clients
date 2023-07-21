
#include <filesystem>
#include <sstream>
#include <QtCore/QStandardPaths>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QWidget>

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "AccountTab.hpp"
#include "LoginDialog.hpp"

#include "andromeda/backend/BackendImpl.hpp" // GetBackend()
#include "andromeda/database/ObjectDatabase.hpp"
using Andromeda::Database::ObjectDatabase;
#include "andromeda/database/SqliteDatabase.hpp"
using Andromeda::Database::SqliteDatabase;
#include "andromeda/filesystem/filedata/CacheOptions.hpp"
using Andromeda::Filesystem::Filedata::CacheOptions;
#include "andromeda-gui/BackendContext.hpp"

namespace AndromedaGui {
namespace Gui {

/*****************************************************/
MainWindow::MainWindow(CacheOptions& cacheOptions) : 
    mDebug(__func__,this),
    mCacheManager(cacheOptions),
    mQtUi(std::make_unique<Ui::MainWindow>())
{
    MDBG_INFO("()");

    std::string dbPath { QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation).toStdString() }; // Qt guarantees never empty
    std::filesystem::create_directories(dbPath);

    dbPath += "/database.s3db"; MDBG_INFO("... dbPath:" << dbPath);
    // TODO shouldn't crash if the DB throws... at least catch exception in main and exit
    // maybe delete, show an alert to the user, try again if the db is corrupt
    mSqlDatabase = std::make_unique<SqliteDatabase>(dbPath);
    mObjDatabase = std::make_unique<ObjectDatabase>(*mSqlDatabase);

    mQtUi->setupUi(this);
}

/*****************************************************/
MainWindow::~MainWindow()
{
    MDBG_INFO("()");

    // need to make sure AccountTab/BackendContexts are deleted before CacheManager
    while (mQtUi->tabAccounts->count() != 0)
    {
        QWidget* accountTab { mQtUi->tabAccounts->widget(0) };
        mQtUi->tabAccounts->removeTab(0); delete accountTab;
    }
}

/*****************************************************/
void MainWindow::show()
{
    MDBG_INFO("()");

    QMainWindow::show(); // base class
    activateWindow(); // bring to front

    if (GetCurrentTab() == nullptr) AddAccount();
}

/*****************************************************/
void MainWindow::closeEvent(QCloseEvent* event)
{
    MDBG_INFO("()");

    if (!event->spontaneous() || GetCurrentTab() == nullptr)
    {
        MDBG_INFO("... closing");
        QMainWindow::closeEvent(event);
    }
    else
    {
        MDBG_INFO("... hiding");
        event->ignore(); hide();
    }
}

/*****************************************************/
void MainWindow::AddAccount()
{
    MDBG_INFO("()");

    LoginDialog loginDialog(*this);
    std::unique_ptr<BackendContext> backendCtx;
    if (loginDialog.CreateBackend(backendCtx))
    {
        backendCtx->GetBackend().SetCacheManager(&mCacheManager);
        AccountTab* accountTab { new AccountTab(*this, std::move(backendCtx)) };

        mQtUi->tabAccounts->setCurrentIndex(
            mQtUi->tabAccounts->addTab(accountTab, accountTab->GetTabName().c_str()));

        mQtUi->actionMount_Storage->setEnabled(true);
        mQtUi->actionUnmount_Storage->setEnabled(true);
        mQtUi->actionBrowse_Storage->setEnabled(true);
        mQtUi->actionRemove_Account->setEnabled(true);
    }
}

/*****************************************************/
void MainWindow::RemoveAccount()
{
    MDBG_INFO("()");

    if (QMessageBox::question(this, "Remove Account", "Are you sure?") == QMessageBox::Yes)
        { MDBG_INFO("... confirmed"); }
    else return; // early return!

    AccountTab* accountTab { GetCurrentTab() };
    if (accountTab != nullptr)
    {
        const int tabIndex { mQtUi->tabAccounts->indexOf(accountTab) };
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
    MDBG_INFO("()");

    AccountTab* accountTab { GetCurrentTab() };
    if (accountTab != nullptr) accountTab->Mount();
}

/*****************************************************/
void MainWindow::UnmountCurrent()
{
    MDBG_INFO("()");

    AccountTab* accountTab { GetCurrentTab() };
    if (accountTab != nullptr) accountTab->Unmount();
}

/*****************************************************/
void MainWindow::BrowseCurrent()
{
    MDBG_INFO("()");

    AccountTab* accountTab { GetCurrentTab() };
    if (accountTab != nullptr) accountTab->Browse();
}

/*****************************************************/
void MainWindow::ShowAbout()
{
    std::stringstream str;
    str << "Andromeda GUI v" << ANDROMEDA_VERSION << std::endl;
    str << "License: GNU GPLv3" << std::endl;

    QMessageBox::about(this, "Andromeda GUI", str.str().c_str());
}

} // namespace Gui
} // namespace AndromedaGui
