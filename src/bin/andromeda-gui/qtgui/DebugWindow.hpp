#ifndef A2GUI_DEBUGWINDOW_H
#define A2GUI_DEBUGWINDOW_H

#include <deque>
#include <memory>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>

#include <QtCore/QTimer>
#include <QtWidgets/QDialog>

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"

namespace Ui { class DebugWindow; }

namespace Andromeda {
    namespace Filesystem { namespace Filedata { class CacheManager; } }
}

namespace AndromedaGui {
namespace QtGui {

class DebugWindow;

/** Extends std::stringbuf to facilitate using a std::ostream
 * to send Debug stream lines to the DebugWindow->AppendDebug() */
class DebugBuffer : public std::stringbuf
{
public:
    explicit DebugBuffer(DebugWindow& window);

protected:
    int sync() override;

private:
    DebugWindow& mWindow;
};

/** A GUI window for displaying Debug module output */
class DebugWindow : public QDialog
{
    Q_OBJECT

public:
    /** Construct with an optional cacheManager pointer (to display stats) */
    explicit DebugWindow(Andromeda::Filesystem::Filedata::CacheManager* cacheManager);

    ~DebugWindow() override;
    DELETE_COPY(DebugWindow)
    DELETE_MOVE(DebugWindow)

    /** Adds a debug line to the buffer to display in the log - THREAD SAFE */
    void AppendDebug(const std::string& str);

protected slots:

    /** Set the maximum number of lines in the debug log - NOT QT THREAD SAFE */
    void SetMaxLines(int lines);
    /** Set whether or not line wrap is enabled (0 is off) - NOT QT THREAD SAFE */
    void SetLineWrap(int wrap);
    /** Sets the debug level (cast to Debug::Level) - NOT QT THREAD SAFE */
    void SetDebugLevel(int level);
    /** Sets the debug filter list (see Debug::SetFilters) - NOT QT THREAD SAFE */
    void SetDebugFilter(const QString& filter);
    /** Updates and repaints the GUI debug log widget from buffered debug lines - NOT QT THREAD SAFE */
    void UpdateDebugLog();
    /** Updates the cache manager and allocator stats labels - NOT QT THREAD SAFE */
    void UpdateCacheStats();

private:
    mutable Andromeda::Debug mDebug;

    /** DebugBuffer to connect std::ostream and this window */
    DebugBuffer mBuffer;
    /** std::ostream to provide to the Debug module */
    std::ostream mStream;

    using LockGuard = std::lock_guard<std::mutex>;

    /** Mutex to protect mCached (stops debug flow until update is done) */
    std::mutex mMutex;
    /** List of cached debug strings not yet painted to the window */
    std::deque<std::string> mCached;

    /** Timer used to update the debug log */
    QTimer mDebugTimer;
    /** Timer used to update the cacheMgr stats */
    QTimer mCacheTimer;

    /** Global cache manager to apply to all mounts (maybe null!) */
    Andromeda::Filesystem::Filedata::CacheManager* mCacheManager;

    std::unique_ptr<Ui::DebugWindow> mQtUi;
};

} // namespace QtGui
} // namespace AndromedaGui

#endif // A2GUI_DEBUGWINDOW_H
