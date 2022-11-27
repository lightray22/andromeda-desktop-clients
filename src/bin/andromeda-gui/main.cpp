
#include <QtWidgets/QApplication>

#include "BackendManager.hpp"
#include "MountManager.hpp"
#include "gui/MainWindow.hpp"
#include "gui/SystemTray.hpp"

#include "andromeda/Utilities.hpp"
using Andromeda::Debug;

#define VERSION "0.1-alpha"

enum class ExitCode
{
    SUCCESS,
    BAD_USAGE
};

/*****************************************************/
int main(int argc, char** argv)
{
    Debug debug("main");

    // TODO parse options on cmdline (e.g. debug)
    debug.SetLevel(Debug::Level::DETAILS); 

    debug << __func__ << "()"; debug.Info();

    QApplication application(argc, argv);
    
    BackendManager backendManager;
    MountManager mountManager;

    MainWindow mainWindow(backendManager, mountManager); 
    SystemTray systemTray(application, mainWindow);

    mainWindow.show();
    systemTray.show();

    int retval = application.exec();

    debug << __func__ << "()... return " << retval; debug.Info();

    return retval;
}
