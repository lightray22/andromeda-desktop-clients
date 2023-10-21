#ifndef A2GUI_SINGLEINSTANCE_H
#define A2GUI_SINGLEINSTANCE_H

#include <string>

#include <QtCore/QLockFile>
#include <QtCore/QString>
#include <QtNetwork/QLocalServer>
#include <QtWidgets/QWidget>

#include "andromeda/Debug.hpp"

namespace AndromedaGui {
namespace QtGui {

/** 
 * Helper for enforcing a single instance per-user
 * Uses a lock file to only allow a single instance
 * Uses a local socket to notify the primary instance when a duplicate starts
 */
class SingleInstance
{
public:

    /** Tries to lock the lock file, starts the single-instance server on success and notifies the existing server on failure */
    explicit SingleInstance(const std::string& lockPath);

    /** Returns true if the lock file failed to lock (there is an existing instance) */
    [[nodiscard]] inline bool isDuplicate() const { return !mLockFile.isLocked(); }

    /** Returns true if notifying the existing server seemed to fail */
    [[nodiscard]] inline bool notifyFailed() const { return mNotifyFailed; }

    /** Registers a window to be shown when notified of a duplicate instance running */
    void ShowOnDuplicate(QWidget& window);

private:

    Andromeda::Debug mDebug;

    /** Socket name to use */
    const QString mServerName;
    /** Lock file to enforce single-instance */
    QLockFile mLockFile;
    /** Server to use for the primary instance */
    QLocalServer mServer;
    /** @see notifyFailed() */
    bool mNotifyFailed { false };
};

} // namespace QtGui
} // namespace AndromedaGui

#endif // A2GUI_UTILITIES_H_
