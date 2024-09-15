
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>

#include <QtCore/QStandardPaths>
#include <QtWidgets/QApplication>

#include "Options.hpp"
using AndromedaGui::Options;
#include "qtgui/MainWindow.hpp"
using AndromedaGui::QtGui::MainWindow;
#include "qtgui/SingleInstance.hpp"
using AndromedaGui::QtGui::SingleInstance;
#include "qtgui/SystemTray.hpp"
using AndromedaGui::QtGui::SystemTray;
#include "qtgui/Utilities.hpp"
using AndromedaGui::QtGui::Utilities;

#include "andromeda/Debug.hpp"
using Andromeda::Debug;
#include "andromeda/database/DatabaseException.hpp"
using Andromeda::Database::DatabaseException;
#include "andromeda/database/ObjectDatabase.hpp"
using Andromeda::Database::ObjectDatabase;
#include "andromeda/backend/SessionStore.hpp"
using Andromeda::Backend::SessionStore;
#include "andromeda/database/SqliteDatabase.hpp"
using Andromeda::Database::SqliteDatabase;
#include "andromeda/database/TableInstaller.hpp"
using Andromeda::Database::TableInstaller;
#include "andromeda/filesystem/filedata/CacheOptions.hpp"
using Andromeda::Filesystem::Filedata::CacheOptions;
#include "andromeda/filesystem/filedata/CacheManager.hpp"
using Andromeda::Filesystem::Filedata::CacheManager;

enum class ExitCode : uint8_t
{
    SUCCESS,
    BAD_USAGE,
    APPDATA,
    INSTANCE
};

/*****************************************************/
int main(int argc, char** argv)
{
    Debug::AddStream(std::cerr);
    Debug debug("main",nullptr); 

    CacheOptions cacheOptions;
    Options options(cacheOptions);

    try
    {
        options.ParseConfig("andromeda-gui");
        options.ParseArgs(static_cast<size_t>(argc), argv);
        options.Validate();
    }
    catch (const Options::ShowHelpException& ex)
    {
        std::cout << Options::HelpText() << std::endl;
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::ShowVersionException& ex)
    {
        std::cout << "version: " << ANDROMEDA_VERSION << std::endl;
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::Exception& ex)
    {
        std::cout << ex.what() << std::endl << std::endl;
        std::cout << Options::HelpText() << std::endl;
        return static_cast<int>(ExitCode::BAD_USAGE);
    }

    DDBG_INFO("()");

    QApplication application(argc, argv);
    application.setApplicationName("andromeda-gui");
    application.setApplicationDisplayName("Andromeda Sync");
    application.setQuitOnLastWindowClosed(false); // handle manually

    const std::string dataPath { QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation).toStdString() }; // Qt guarantees never empty

    DDBG_INFO("... init dataPath:" << dataPath);
    try { std::filesystem::create_directories(dataPath); }
    catch (const std::filesystem::filesystem_error& ex)
    {
        DDBG_ERROR("... " << ex.what());
        const std::string msg { "Failed to create appdata directory." };
        Utilities::criticalBox(nullptr, "Initialize Error", msg, ex);
        return static_cast<int>(ExitCode::APPDATA);
    }
    
    const std::string lockPath { dataPath+"/database.qtlock" };
    SingleInstance instanceMgr(lockPath);
    if (instanceMgr.isDuplicate())
    {
        if (instanceMgr.notifyFailed())
            QMessageBox::critical(nullptr, "Initialize Error", "Andromeda is already running!");
        return static_cast<int>(ExitCode::INSTANCE);
    }

    const std::string dbPath { dataPath+"/database.s3db" };
    DDBG_INFO("... init dbPath:" << dbPath);
    std::unique_ptr<SqliteDatabase> sqlDatabase;
    std::unique_ptr<ObjectDatabase> objDatabase;
    try
    {
        sqlDatabase = std::make_unique<SqliteDatabase>(dbPath);
        objDatabase = std::make_unique<ObjectDatabase>(*sqlDatabase);

        DDBG_INFO("... checking database tables");
        TableInstaller tableInst(*objDatabase);
        tableInst.InstallTable<SessionStore>();
    }
    catch (const DatabaseException& ex)
    {
        DDBG_ERROR("... " << ex.what());
        std::stringstream str;
        str << "Failed to initialize the database. This is probably a bug, please report." << std::endl;
        str << "Previously saved accounts are unavailable, and new ones will not be saved.";
        Utilities::warningBox(nullptr, "Database Error", str.str(), ex);
    }

    std::unique_ptr<CacheManager> cacheMgr;
    if (!cacheOptions.disable) cacheMgr = 
        std::make_unique<CacheManager>(cacheOptions);

    MainWindow mainWindow(application, cacheMgr.get(), objDatabase.get()); 
    SystemTray systemTray(application, mainWindow);

    instanceMgr.ShowOnDuplicate(mainWindow);

    systemTray.show();
    mainWindow.show();

    const int retval = application.exec();
    DDBG_INFO("... return " << retval);
    return retval;
}
