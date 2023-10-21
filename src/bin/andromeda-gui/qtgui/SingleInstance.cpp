
#include <QtNetwork/QLocalSocket>

#include "SingleInstance.hpp"
#include "Utilities.hpp"

namespace AndromedaGui {
namespace QtGui {

/*****************************************************/
SingleInstance::SingleInstance(const std::string& lockPath) :
    mDebug(__func__, this),
    mServerName(Utilities::hash16(lockPath).toHex()),
    mLockFile(lockPath.c_str())
{
    MDBG_INFO("(lockPath:" << lockPath << ")"
        << " mServerName:" << mServerName.toStdString());
    mLockFile.setStaleLockTime(0);

    if (mLockFile.tryLock())
    { 
        MDBG_INFO("... lock aquired, starting single-instance server!");

        mServer.setSocketOptions(QLocalServer::UserAccessOption);
        mServer.removeServer(mServerName); // in case of a previous crash

        if (!mServer.listen(mServerName))
            { MDBG_ERROR("... failed to start server"); }
    }
    else 
    { 
        MDBG_INFO("... single-instance lock failed! already running?");

        QLocalSocket sock;
        sock.connectToServer(mServerName);
        mNotifyFailed = !sock.waitForConnected();

        MDBG_INFO("... " << (mNotifyFailed?"failed to notify":"notified") << " existing instance!");
    }
}

/*****************************************************/
void SingleInstance::ShowOnDuplicate(QWidget& window)
{
    // if we get a connection to the single-instance server, show the window
    QObject::connect(&mServer, &QLocalServer::newConnection, [&]()
    {
        MDBG_INFO("... new single-instance socket connection"); // NOLINT(bugprone-lambda-function-name)
        QLocalSocket& clientSocket = *mServer.nextPendingConnection();
        Utilities::fullShow(window);
        clientSocket.abort();
    });
}

} // namespace QtGui
} // namespace AndromedaGui
