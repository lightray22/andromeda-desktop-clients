
#include <QApplication>

#include "MainWindow.hpp"

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

    debug.SetLevel(Debug::Level::DETAILS); // TODO temp
    debug << __func__ << "()"; debug.Info();

    QApplication a(argc, argv);
    
    MainWindow w;
    w.show();

    return a.exec();

    //debug.Info("returning success...");
    //return static_cast<int>(ExitCode::SUCCESS);
}
