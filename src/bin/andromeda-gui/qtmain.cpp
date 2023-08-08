
#include <iostream>
#include <QtWidgets/QApplication>

#include "Options.hpp"
using AndromedaGui::Options;
#include "qtgui/MainWindow.hpp"
using AndromedaGui::QtGui::MainWindow;
#include "qtgui/SystemTray.hpp"
using AndromedaGui::QtGui::SystemTray;

#include "andromeda/Debug.hpp"
using Andromeda::Debug;
#include "andromeda/filesystem/filedata/CacheOptions.hpp"
using Andromeda::Filesystem::Filedata::CacheOptions;

enum class ExitCode
{
    SUCCESS,
    BAD_USAGE
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

    MainWindow mainWindow(cacheOptions); 
    SystemTray systemTray(application, mainWindow);

    systemTray.show();
    mainWindow.show();

    int retval = application.exec();

    DDBG_INFO("... return " << retval);

    return retval;
}
