
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>

#include <QtCore/QLockFile>
#include <QtCore/QStandardPaths>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>
#include <QtWidgets/QApplication>

#include "Options.hpp"
using AndromedaGui::Options;
#include "qtgui/Utilities.hpp"
using AndromedaGui::QtGui::Utilities;
#include "qtgui/MainWindow.hpp"
using AndromedaGui::QtGui::MainWindow;
#include "qtgui/SystemTray.hpp"
using AndromedaGui::QtGui::SystemTray;

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

enum class ExitCode
{
    SUCCESS,
    BAD_USAGE,
    APPDATA,
    INSTANCE
};

/*****************************************************/
int main(int argc, char** argv)
{
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

    const std::string dataPath { QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation).toStdString() }; // Qt guarantees never empty

    DDBG_INFO("... init dataPath:" << dataPath);
    try { std::filesystem::create_directories(dataPath); }
    catch (const std::filesystem::filesystem_error& ex)
    {
        DDBG_ERROR("... " << ex.what());
        const std::string msg { "Failed to create appdata directory." };
        Utilities::criticalBox(nullptr, "Initialize Error", msg.c_str(), ex);
        return static_cast<int>(ExitCode::APPDATA);
    }
    
    const std::string dbPath { dataPath+"/database.s3db" };
    const std::string lockPath { dbPath+".qtlock" };

    // start a socket to coordinate SingleInstance
    const QString serverName { Utilities::hash16(lockPath.c_str()).toHex() };

    QLocalServer instanceServer;
    QLockFile dbLock(lockPath.c_str());
    if (dbLock.tryLock())
    { 
        DDBG_INFO("... starting single-instance server: " << serverName.toStdString());
        instanceServer.setSocketOptions(QLocalServer::UserAccessOption);
        instanceServer.removeServer(serverName); // in case of a previous crash
        if (!instanceServer.listen(serverName))
            { DDBG_ERROR("... failed to start server"); }
    }
    else 
    { 
        DDBG_INFO("... single-instance lock failed! already running?");
        QLocalSocket sock; sock.connectToServer(serverName);
        if (!sock.waitForConnected()) // must be busy?
        {
            DDBG_INFO("... didn't connect to socket, show message box");
            QMessageBox::critical(nullptr, "Initialize Error", "Andromeda is already running!");
        }
        else { DDBG_INFO("... notified existing instance! returning"); }
        return static_cast<int>(ExitCode::INSTANCE);
    }

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

    CacheManager cacheManager(cacheOptions);

    MainWindow mainWindow(cacheManager, objDatabase.get()); 
    SystemTray systemTray(application, mainWindow);

    // if we get a connection to the single-instance server, show the window
    QObject::connect(&instanceServer, &QLocalServer::newConnection, [&]()
    {
        DDBG_INFO("... new single-instance socket connection");
        QLocalSocket& clientSocket = *instanceServer.nextPendingConnection();
        mainWindow.fullShow();
        clientSocket.abort();
    });

    systemTray.show();
    mainWindow.show();

    int retval = application.exec();
    DDBG_INFO("... return " << retval);
    return retval;
}
