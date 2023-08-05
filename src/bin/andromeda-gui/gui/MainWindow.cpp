
#include <filesystem>
#include <sstream>
#include <QtCore/QStandardPaths>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QWidget>

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "AccountTab.hpp"
#include "ExceptionBox.hpp"
#include "LoginDialog.hpp"

#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/backend/SessionStore.hpp"
using Andromeda::Backend::SessionStore;
#include "andromeda/database/DatabaseException.hpp"
using Andromeda::Database::DatabaseException;
#include "andromeda/database/ObjectDatabase.hpp"
using Andromeda::Database::ObjectDatabase;
#include "andromeda/database/SqliteDatabase.hpp"
using Andromeda::Database::SqliteDatabase;
#include "andromeda/database/TableInstaller.hpp"
using Andromeda::Database::TableInstaller;
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

    mQtUi->setupUi(this);

    try
    {
        std::string dbPath { QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation).toStdString() }; // Qt guarantees never empty

        try { std::filesystem::create_directories(dbPath); }
        catch (const std::filesystem::filesystem_error& ex)
            { throw DatabaseException(ex.what()); }
        
        dbPath += "/database.s3db"; MDBG_INFO("... init dbPath:" << dbPath);
        mSqlDatabase = std::make_unique<SqliteDatabase>(dbPath);
        mObjDatabase = std::make_unique<ObjectDatabase>(*mSqlDatabase);

        MDBG_INFO("... installing database tables");
        TableInstaller tableInst(*mObjDatabase);
        tableInst.InstallTable<SessionStore>();

        MDBG_INFO("... loading existing sessions");
        for (SessionStore* session : SessionStore::LoadAll(*mObjDatabase))
        {
            std::unique_ptr<BackendContext> backendCtx {
                std::make_unique<BackendContext>(*session) };

            backendCtx->GetBackend().SetCacheManager(&mCacheManager);
            AddAccountTab(std::move(backendCtx));
        }
    }
    catch (const DatabaseException& ex)
    {
        MDBG_ERROR("... " << ex.what());

        std::stringstream str;
        str << "Failed to initialize the database. This is probably a bug, please report." << std::endl;
        str << "Previously saved accounts are unavailable, and new ones will not be saved.";
        ExceptionBox::warning(this, "Database Error", str.str(), ex);
    }
}

/*****************************************************/
MainWindow::~MainWindow()
{
    MDBG_INFO("()");

    // need to make sure AccountTab/BackendContexts are deleted before CacheManager
    while (mQtUi->tabAccounts->count() != 0)
    {
        QWidget* accountTab { mQtUi->tabAccounts->widget(0) };
        mQtUi->tabAccounts->removeTab(0); delete accountTab; // NOLINT(cppcoreguidelines-owning-memory)
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
        try { if (mObjDatabase) backendCtx->StoreSession(*mObjDatabase); }
        catch (const DatabaseException& ex)
        {
            MDBG_ERROR("... " << ex.what());
            ExceptionBox::warning(this, "Database Error", "Failed to add the account to the database. This is probably a bug, please report.", ex);
        }

        backendCtx->GetBackend().SetCacheManager(&mCacheManager);
        AddAccountTab(std::move(backendCtx));
    }
}

/*****************************************************/
void MainWindow::AddAccountTab(std::unique_ptr<BackendContext> backendCtx)
{
    AccountTab* accountTab { new AccountTab(*this, std::move(backendCtx)) };

    mQtUi->tabAccounts->setCurrentIndex(
        mQtUi->tabAccounts->addTab(accountTab, accountTab->GetTabName().c_str()));

    mQtUi->actionMount_Storage->setEnabled(true);
    mQtUi->actionUnmount_Storage->setEnabled(true);
    mQtUi->actionBrowse_Storage->setEnabled(true);
    mQtUi->actionRemove_Account->setEnabled(true);
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
        SessionStore* session { accountTab->GetBackendContext().GetSessionStore() };
        
        try { if (session != nullptr) mObjDatabase->DeleteObject(*session); }
        catch (const DatabaseException& ex)
        {
            MDBG_ERROR("... " << ex.what());
            ExceptionBox::warning(this, "Database Error", "Failed to remove the account from the database. This is probably a bug, please report.", ex);
        }

        const int tabIndex { mQtUi->tabAccounts->indexOf(accountTab) };
        mQtUi->tabAccounts->removeTab(tabIndex); delete accountTab; // NOLINT(cppcoreguidelines-owning-memory)
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
