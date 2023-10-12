#ifndef A2GUI_DEBUGWINDOW_H
#define A2GUI_DEBUGWINDOW_H

#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <QtWidgets/QDialog>

#include "andromeda/common.hpp"

namespace Ui { class DebugWindow; }

namespace AndromedaGui {
namespace QtGui {

class DebugWindow;

// TODO !! comments
class DebugBuffer : public QObject, public std::stringbuf
{
    Q_OBJECT

public:
    DebugBuffer(DebugWindow& window);

signals:
    void DebugOutput(const std::string& str);

protected:
    virtual int sync();

private:
    DebugWindow& mWindow;
};

// TODO !! comments
class DebugWindow : public QDialog
{
    Q_OBJECT

public:
    explicit DebugWindow();

    ~DebugWindow() override;
    DELETE_COPY(DebugWindow)
    DELETE_MOVE(DebugWindow)

protected slots:
    void AppendText(const std::string& str);

private:
    DebugBuffer mBuffer;
    std::ostream mStream;

    std::unique_ptr<Ui::DebugWindow> mQtUi;
};

} // namespace QtGui
} // namespace AndromedaGui

#endif // A2GUI_DEBUGWINDOW_H
