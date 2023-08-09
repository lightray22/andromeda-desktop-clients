#ifndef A2GUI_UTILITIES_H
#define A2GUI_UTILITIES_H

#include <sstream>

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QWidget>

namespace AndromedaGui {
namespace QtGui {

/** Qt Helper functions */
struct Utilities
{
    Utilities() = delete; // static only

    /** Runs QMessageBox::critical with the exception text at the bottom */
    static inline void criticalBox(QWidget* parent, const std::string& title, const std::string& msg, const std::exception& ex)
    {
        std::stringstream str; str << msg << std::endl << std::endl; str << ex.what();
        QMessageBox::critical(parent, title.c_str(), str.str().c_str());
    }

    /** Runs QMessageBox::warning with the exception text at the bottom */
    static inline void warningBox(QWidget* parent, const std::string& title, const std::string& msg, const std::exception& ex)
    {
        std::stringstream str; str << msg << std::endl << std::endl; str << ex.what();
        QMessageBox::warning(parent, title.c_str(), str.str().c_str());
    }

    /** Shows, raises, focuses the widget (even on Windows) */
    static void fullShow(QWidget& widget);

    /** Returns a 16-byte hash of the given string */
    static QByteArray hash16(const std::string& str);
};

} // namespace QtGui
} // namespace AndromedaGui

#endif // A2GUI_UTILITIES_H_
