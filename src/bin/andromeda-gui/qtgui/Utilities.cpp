
#if WIN32
#include <windows.h>
#endif // WIN32

#include <QtCore/QCryptographicHash>

#include "Utilities.hpp"

namespace AndromedaGui {
namespace QtGui {

/*****************************************************/
void Utilities::fullShow(QWidget& widget)
{
    widget.show(); // visible
    widget.raise(); // bring to top
    widget.activateWindow(); // focus

    #if WIN32
        // this restores the window if minimized to the task bar
        const HWND hwnd { reinterpret_cast<HWND>(widget.winId()) };
        if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
    #endif // WIN32
}

/*****************************************************/
QByteArray Utilities::hash16(const std::string& str)
{
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(QByteArray::fromStdString(str));
    return hash.result();
}

} // namespace QtGui
} // namespace AndromedaGui
