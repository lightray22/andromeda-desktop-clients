
#include "MainWindow.hpp"
#include "./ui_MainWindow.h"

#include "andromeda-fuse/FuseAdapter.hpp"
using AndromedaFuse::FuseAdapter;

#include "andromeda/HTTPRunner.hpp"
using Andromeda::HTTPRunner;
#include "andromeda/Backend.hpp"
using Andromeda::Backend;
#include "andromeda/Config.hpp"
using Andromeda::Config;
#include "andromeda/Utilities.hpp"
using Andromeda::Utilities;

#include "andromeda/fsitems/Folder.hpp"
using Andromeda::FSItems::Folder;
#include "andromeda/fsitems/folders/SuperRoot.hpp"
using Andromeda::FSItems::Folders::SuperRoot;

/*****************************************************/
MainWindow::MainWindow(): QMainWindow(),
    mUi(std::make_unique<Ui::MainWindow>()),
    mDebug("MainWindow")
{
    mDebug << __func__ << "()"; mDebug.Info();

    mUi->setupUi(this);
}

/*****************************************************/
MainWindow::~MainWindow()
{ 
    Unmount();
}

/*****************************************************/
void MainWindow::Mount()
{
    Unmount();

    mDebug << __func__ << "()"; mDebug.Info();

    std::string apiurl    (mUi->input_apiurl->text().toStdString());
    std::string mountpath (mUi->input_mountpath->text().toStdString());
    std::string username  (mUi->input_username->text().toStdString());
    std::string password  (mUi->input_password->text().toStdString());

    mDebug << __func__ << "... apiurl:(" << apiurl << ") mountpath:(" << mountpath
        << ") username:(" << username << ") password:(" << password << ")"; mDebug.Info();

    mFuseThread = std::make_unique<std::thread>([&,apiurl,mountpath,username,password]
    {
        HTTPRunner::Options httpOptions;
        Config::Options configOptions;
        FuseAdapter::Options fuseOptions;

        std::vector<std::string> parts {
            Utilities::explode(apiurl, "/", 2, 2) };

        if (parts.size() != 2) { mDebug.Error("invalid apiurl!"); return; }

        std::string apiPath = "/"+parts[1];
        std::string apiHostname = parts[0];

        std::unique_ptr<Backend::Runner> runner { 
            std::make_unique<HTTPRunner>(
                apiHostname, apiPath, httpOptions) };

        if (mountpath.empty()) { mDebug.Error("empty mountpath!"); return; } // TODO enforce in fuselib?
        fuseOptions.mountPath = mountpath;
        
        Backend backend(*runner);
        std::unique_ptr<Folder> folder;
        
        try
        {
            backend.Initialize(configOptions);
            backend.AuthInteractive(username, password);

            folder = std::make_unique<SuperRoot>(backend);
        }
        catch (const Utilities::Exception& ex)
        {
            std::cout << ex.what() << std::endl; return;
        }

        try { (dynamic_cast<HTTPRunner&>(*runner)).EnableRetry(); }
        catch (const std::bad_cast& ex) { } // no retries during init

        try
        {
            FuseAdapter fuseAdapter(*folder, fuseOptions);
        }
        catch (const FuseAdapter::Exception& ex)
        {
            std::cout << ex.what() << std::endl; return;
        }
    });
}

/*****************************************************/
void MainWindow::Unmount()
{
    mDebug << __func__ << "()"; mDebug.Info();

    if (mFuseThread) 
    {
        mFuseThread->join(); // TODO signal it to stop
        mFuseThread.reset();
    }

    // need to refactor FuseAdapter to do the threading itself so we can do .Stop() on it
    // then it can do its own fuse_exit(fuse*) and return
}
