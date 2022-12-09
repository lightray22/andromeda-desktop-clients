
#include <iostream>
#include <QtWidgets/QApplication>

#include "Options.hpp"

#include "gui/MainWindow.hpp"
#include "gui/SystemTray.hpp"

#include "andromeda/Debug.hpp"
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

    // if foreground, you must want debug
    Debug::SetLevel(Debug::Level::ERRORS); 

    Options options;

    try
    {
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
        std::cout << "version: " << VERSION << std::endl;
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::Exception& ex)
    {
        std::cout << ex.what() << std::endl << std::endl;
        std::cout << Options::HelpText() << std::endl;
        return static_cast<int>(ExitCode::BAD_USAGE);
    }

    Debug::SetLevel(options.GetDebugLevel());

    debug << __func__ << "()"; debug.Info();

    QApplication application(argc, argv);

    MainWindow mainWindow; 
    SystemTray systemTray(application, mainWindow);

    mainWindow.show();
    systemTray.show();

    int retval = application.exec();

    debug << __func__ << "... return " << retval; debug.Info();

    return retval;
}
