#ifndef A2GUI_LOGINDIALOG_HPP
#define A2GUI_LOGINDIALOG_HPP

#include <memory>
#include <QtWidgets/QDialog>
#include <QtWidgets/QWidget>

#include "andromeda/Debug.hpp"

namespace AndromedaGui {
class BackendContext;

namespace Gui {

namespace Ui { class LoginDialog; }

/** The window for logging in (creating backend resources) */
class LoginDialog : public QDialog
{
    Q_OBJECT

public:

    explicit LoginDialog(QWidget& parent);

    ~LoginDialog() override;
    LoginDialog(const LoginDialog&) = delete; // no copy
    LoginDialog& operator=(const LoginDialog&) = delete;
    LoginDialog(LoginDialog&&) = delete; // no move
    LoginDialog& operator=(LoginDialog&&) = delete;

    /** 
     * Runs QDialog::exec() and gives the created backend 
     * @param[out] backend backend gets created here
     */
    int CreateBackend(std::unique_ptr<BackendContext>& backend);

public slots:

    void accept() override;

private:

    mutable Andromeda::Debug mDebug;

    /** The backend context created by the user */
    std::unique_ptr<BackendContext> mBackendContext;

    std::unique_ptr<Ui::LoginDialog> mQtUi;
};

} // namespace Gui
} // namespace AndromedaGui

#endif // LOGINDIALOG_HPP
