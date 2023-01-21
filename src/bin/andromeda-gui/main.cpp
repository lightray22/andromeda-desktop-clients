
#include <iostream>
#include <QtWidgets/QApplication>

#include "Options.hpp"
using AndromedaGui::Options;
#include "gui/MainWindow.hpp"
using AndromedaGui::Gui::MainWindow;
#include "gui/SystemTray.hpp"
using AndromedaGui::Gui::SystemTray;

#include "andromeda/Debug.hpp"
using Andromeda::Debug;

enum class ExitCode
{
    SUCCESS,
    BAD_USAGE
};

/*****************************************************/
int main(int argc, char** argv)
{
    Debug debug("main",nullptr); 

    Options options;

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

    MainWindow mainWindow; 
    SystemTray systemTray(application, mainWindow);

    mainWindow.show();
    systemTray.show();

    int retval = application.exec();

    DDBG_INFO("... return " << retval);

    return retval;
}
