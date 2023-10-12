#include "DebugWindow.hpp"
#include "ui_DebugWindow.h"

#include "andromeda/Debug.hpp"
using Andromeda::Debug;

namespace AndromedaGui {
namespace QtGui {

// TODO !! add UI buttons - set level, clear, etc.
// TODO !! need to limit the amount of text, gets slow...

/*****************************************************/
DebugWindow::DebugWindow() :
    mBuffer(*this), mStream(&mBuffer),
    mQtUi(std::make_unique<Ui::DebugWindow>())
{
    Debug::AddStream(mStream);

    // TODO !! handle closing window, deregistering debug

    QObject::connect(
        &mBuffer, &DebugBuffer::DebugOutput,
        this, &DebugWindow::AppendText);

    mQtUi->setupUi(this);
}

/*****************************************************/
DebugWindow::~DebugWindow() { }

/*****************************************************/
void DebugWindow::AppendText(const std::string& str)
{
    mQtUi->plainTextEdit->moveCursor(QTextCursor::End);
    mQtUi->plainTextEdit->insertPlainText(str.c_str());
}

/*****************************************************/
DebugBuffer::DebugBuffer(DebugWindow& window) :
    mWindow(window) { }

/*****************************************************/
int DebugBuffer::sync()
{
    emit DebugOutput(str());
    str(""); return 0;
}

} // namespace QtGui
} // namespace AndromedaGui
