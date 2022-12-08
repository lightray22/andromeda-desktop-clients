#ifndef A2GUI_LOGINDIALOG_HPP
#define A2GUI_LOGINDIALOG_HPP

#include <memory>
#include <QtWidgets/QDialog>
#include <QtWidgets/QWidget>

#include "andromeda/Debug.hpp"

class BackendContext;
namespace Ui { class LoginDialog; }

/** The window for logging in (creating backend resources) */
class LoginDialog : public QDialog
{
    Q_OBJECT

public:

    explicit LoginDialog(QWidget& parent);

    virtual ~LoginDialog();

    /** Returns and takes ownership of a unique_ptr to the created backend */
    std::unique_ptr<BackendContext> TakeBackend();

public slots:

    void accept() override;

private:

    /** The backend context created by the user */
    std::unique_ptr<BackendContext> mBackendContext;

    std::unique_ptr<Ui::LoginDialog> mQtUi;

    Andromeda::Debug mDebug;
};

#endif // LOGINDIALOG_HPP
