
#include <chrono>
#include <QtCore/QTextStream>

#include "DebugWindow.hpp"
#include "ui_DebugWindow.h"

#include "andromeda/Debug.hpp"
using Andromeda::Debug;
#include "andromeda/StringUtil.hpp"
using Andromeda::StringUtil;
#include "andromeda/filesystem/filedata/CacheManager.hpp"
using Andromeda::Filesystem::Filedata::CacheManager;
#include "andromeda/filesystem/filedata/CachingAllocator.hpp"
using Andromeda::Filesystem::Filedata::CachingAllocator;

namespace AndromedaGui {
namespace QtGui {

/*****************************************************/
DebugBuffer::DebugBuffer(DebugWindow& window) : mWindow(window) { }

/*****************************************************/
int DebugBuffer::sync()
{
    mWindow.AppendDebug(str());
    str(""); return 0;
}

// THE LOG UPDATE DESIGN - we add debug log entries to a cache until the timer goes off to update
// the debug window, at which point we grab a mutex and send all lines to the widget.  Further
// debug coming in has to wait for the mutex which prevents the UI from falling behind.
// This gives the best balance between the UI update rate and overall application speed.
// BAD IDEAS 1) DebugBuffer could use a Qt signal -> DebugWindow slot to inform debug, but this
// floods Qt with way too many events and the UI would freeze completely while debug was flowing
// 2) so force a repaint on every signal? the UI refreshes quickly but now falls way behind the real log
// 3) add a mutex? We don't fall behind but the whole application is slow as we are repainting for every line

/*****************************************************/
DebugWindow::DebugWindow(CacheManager* cacheManager) :
    mDebug(__func__,this),
    mBuffer(*this), mStream(&mBuffer),
    mCacheManager(cacheManager),
    mQtUi(std::make_unique<Ui::DebugWindow>())
{
    mQtUi->setupUi(this);
    SetMaxLines(mQtUi->maxLinesSpinBox->value());
    SetLineWrap(mQtUi->wordWrapCheckBox->checkState());

    Debug::AddStream(mStream);
    SetDebugFilter(mQtUi->filtersLineEdit->text());
    SetDebugLevel(mQtUi->levelComboBox->currentIndex());
    MDBG_INFO("()");

    QObject::connect(&mDebugTimer, &QTimer::timeout, this, &DebugWindow::UpdateDebugLog);
    mDebugTimer.start(std::chrono::milliseconds(50)); // hardcoded 20Hz

    QObject::connect(&mCacheTimer, &QTimer::timeout, this, &DebugWindow::UpdateCacheStats);
    mCacheTimer.start(std::chrono::milliseconds(250)); // hardcoded 4Hz
}

/*****************************************************/
DebugWindow::~DebugWindow()
{
    Debug::RemoveStream(mStream);
    MDBG_INFO("()");
}

/*****************************************************/
void DebugWindow::AppendDebug(const std::string& str)
{
    // obviously, don't call MDBG_INFO here...
    const LockGuard llock(mMutex);
    mCached.emplace_back(str);
}

/*****************************************************/
void DebugWindow::UpdateDebugLog()
{
    const LockGuard llock(mMutex);
    if (mCached.empty()) return; // don't move cursor

    mQtUi->plainTextEdit->moveCursor(QTextCursor::End);
    for (const std::string& str : mCached)
        mQtUi->plainTextEdit->insertPlainText(str.c_str());
    mQtUi->plainTextEdit->moveCursor(QTextCursor::End);

    mQtUi->plainTextEdit->repaint();
    mCached.clear();
}

/*****************************************************/
void DebugWindow::SetMaxLines(int lines)
{
    MDBG_INFO("(lines:" << lines << ")");
    mQtUi->plainTextEdit->setMaximumBlockCount(lines+1);
}

/*****************************************************/
void DebugWindow::SetLineWrap(int wrap)
{
    MDBG_INFO("(wrap:" << wrap << ")");
    mQtUi->plainTextEdit->setLineWrapMode(
        wrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

/*****************************************************/
void DebugWindow::SetDebugLevel(int level)
{
    MDBG_INFO("(level:" << level << ")");
    Debug::SetLevel(static_cast<Debug::Level>(level), mStream);
}

/*****************************************************/
void DebugWindow::SetDebugFilter(const QString& filter)
{
    MDBG_INFO("(filter:" << filter.toStdString() << ")");
    Debug::SetFilters(filter.toStdString(), mStream);
}

/*****************************************************/
void DebugWindow::UpdateCacheStats()
{
    if (!mCacheManager) return;

    const CacheManager::Stats cacheStats { mCacheManager->GetStats() };
    QString cacheText; QTextStream(&cacheText)
        << "currentTotal: " << StringUtil::bytesToStringF(cacheStats.currentTotal).c_str() 
            << " (" << cacheStats.totalPages << " pages)"
        << ", currentDirty: " << StringUtil::bytesToStringF(cacheStats.currentDirty).c_str() 
            << " (" << StringUtil::bytesToStringF(cacheStats.dirtyLimit).c_str() << " limit)"
            << " (" << cacheStats.dirtyPages << " pages)";
    mQtUi->cacheMgrStats->setText(cacheText);

    const CachingAllocator::Stats allocStats { mCacheManager->GetPageAllocator().GetStats() };
    QString allocText; QTextStream(&allocText)
        << "curAlloc: " << StringUtil::bytesToStringF(allocStats.curAlloc).c_str()
            << " (" << StringUtil::bytesToStringF(allocStats.maxAlloc).c_str() << " max)"
        << ", curFree: " << StringUtil::bytesToStringF(allocStats.curFree).c_str()
        << ", allocs: " << allocStats.allocs << ", recycles: " << allocStats.recycles;
    mQtUi->cacheAllocStats->setText(allocText);
}

} // namespace QtGui
} // namespace AndromedaGui
