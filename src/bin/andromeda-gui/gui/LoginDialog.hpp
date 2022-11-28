#ifndef A2GUI_LOGINDIALOG_HPP
#define A2GUI_LOGINDIALOG_HPP

#include <memory>
#include <QtWidgets/QDialog>
#include <QtWidgets/QWidget>

#include "andromeda/Utilities.hpp"

class BackendContext;
namespace Ui { class LoginDialog; }

class LoginDialog : public QDialog
{
    Q_OBJECT

public:

    explicit LoginDialog(QWidget& parent);

    virtual ~LoginDialog();

    std::unique_ptr<BackendContext> TakeBackend();

public slots:

    void accept() override;

private:

    std::unique_ptr<BackendContext> mBackendContext;

    std::unique_ptr<Ui::LoginDialog> mQtUi;

    Andromeda::Debug mDebug;
};

#endif // LOGINDIALOG_HPP
