
#include <sstream>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QWidget>

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "AccountTab.hpp"
#include "DebugWindow.hpp"
#include "LoginDialog.hpp"
#include "Utilities.hpp"

#include "andromeda/backend/BackendException.hpp"
using Andromeda::Backend::BackendException;
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/backend/SessionStore.hpp"
using Andromeda::Backend::SessionStore;
#include "andromeda/database/DatabaseException.hpp"
using Andromeda::Database::DatabaseException;
#include "andromeda/database/ObjectDatabase.hpp"
using Andromeda::Database::ObjectDatabase;
#include "andromeda/filesystem/filedata/CacheManager.hpp"
using Andromeda::Filesystem::Filedata::CacheManager;
#include "andromeda-gui/BackendContext.hpp"

namespace AndromedaGui {
namespace QtGui {

/*****************************************************/
MainWindow::MainWindow(CacheManager* cacheManager, ObjectDatabase* objDatabase) : 
    mDebug(__func__,this),
    mCacheManager(cacheManager),
    mObjDatabase(objDatabase),
    mQtUi(std::make_unique<Ui::MainWindow>())
{
    MDBG_INFO("()");

    mQtUi->setupUi(this);

    if (mObjDatabase != nullptr)
    {
        MDBG_INFO("... loading existing sessions");
        for (SessionStore* session : SessionStore::LoadAll(*mObjDatabase))
            TryLoadAccount(*session);
    }

    if (GetCurrentTab() == nullptr) AddAccount();
    else mQtUi->tabAccounts->setCurrentIndex(0);
}

/*****************************************************/
MainWindow::~MainWindow()
{
    MDBG_INFO("()");
}

/*****************************************************/
void MainWindow::fullShow()
{
    Utilities::fullShow(*this);
}

/*****************************************************/
void MainWindow::closeEvent(QCloseEvent* event)
{
    MDBG_INFO("()");

    if (!event->spontaneous() || GetCurrentTab() == nullptr)
    {
        MDBG_INFO("... closing");

        // destruct tabs now before the window disappears
        while (mQtUi->tabAccounts->count() != 0)
            RemoveAccountTab(0);

        if (mDebugWindow)
            mDebugWindow->close();

        QMainWindow::closeEvent(event);
    }
    else
    {
        MDBG_INFO("... hiding");
        event->ignore(); hide();
    }
}

/*****************************************************/
void MainWindow::TryLoadAccount(SessionStore& session)
{
    MDBG_INFO("(serverUrl:" << session.GetServerUrl() << ")");

    try
    {
        std::unique_ptr<BackendContext> backendCtx {
            std::make_unique<BackendContext>(session) };

        backendCtx->GetBackend().SetCacheManager(mCacheManager);
        AddAccountTab(std::move(backendCtx));
    }
    catch (const BackendException& ex)
    {
        MDBG_ERROR("... " << ex.what());
        const std::string msg("Failed to connect to the server at ");
        Utilities::warningBox(this, "Connection Error", msg+session.GetServerUrl(), ex);
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
            const std::string msg { "Failed to add the account to the database. This is probably a bug, please report." };
            Utilities::warningBox(this, "Database Error", msg.c_str(), ex);
        }

        backendCtx->GetBackend().SetCacheManager(mCacheManager);
        AddAccountTab(std::move(backendCtx));
    }
}

/*****************************************************/
void MainWindow::AddAccountTab(std::unique_ptr<BackendContext> backendCtx)
{
    MDBG_INFO("()");

    AccountTab* accountTab { new AccountTab(*this, std::move(backendCtx)) };

    const int idx = mQtUi->tabAccounts->addTab(accountTab, accountTab->GetTabName().c_str());
    MDBG_INFO("... idx:" << idx << " accountTab:" << accountTab);
    mQtUi->tabAccounts->setCurrentIndex(idx);

    MDBG_INFO("... size:" << mQtUi->tabAccounts->count() << " curIdx:" << mQtUi->tabAccounts->currentIndex() << " curWdgt:" << mQtUi->tabAccounts->currentWidget());
    
    mQtUi->actionMount_Storage->setEnabled(true);
    mQtUi->actionUnmount_Storage->setEnabled(true);
    mQtUi->actionBrowse_Storage->setEnabled(true);
    mQtUi->actionRemove_Account->setEnabled(true);
}

/*****************************************************/
void MainWindow::RemoveAccountTab(const int tabIndex)
{
    MDBG_INFO("(tabIndex:" << tabIndex << ")");

    QWidget* const accountTab { mQtUi->tabAccounts->widget(tabIndex) };
    mQtUi->tabAccounts->removeTab(tabIndex);
    delete accountTab; // NOLINT(cppcoreguidelines-owning-memory)
}

/*****************************************************/
void MainWindow::RemoveAccountTab(AccountTab* const accountTab)
{
    MDBG_INFO("(accountTab:" << accountTab << ")");

    const int tabIndex { mQtUi->tabAccounts->indexOf(accountTab) };
    mQtUi->tabAccounts->removeTab(tabIndex);
    delete accountTab; // NOLINT(cppcoreguidelines-owning-memory)
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
            const std::string msg { "Failed to remove the account from the database. This is probably a bug, please report." };
            Utilities::warningBox(this, "Database Error", msg.c_str(), ex);
        }

        RemoveAccountTab(accountTab);
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
    MDBG_INFO("()");

    std::stringstream str;
    str << "Andromeda GUI v" << ANDROMEDA_VERSION << std::endl;
    str << "License: GNU GPLv3" << std::endl;

    QMessageBox::about(this, "About", str.str().c_str());
}

/*****************************************************/
void MainWindow::ShowDebug()
{
    MDBG_INFO("()");

    if (!mDebugWindow)
        mDebugWindow = std::make_unique<DebugWindow>();
    mDebugWindow->show();

    // TODO !! how to erase ptr after closing window?
}

} // namespace QtGui
} // namespace AndromedaGui
