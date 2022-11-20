#ifndef A2GUI_LOGINDIALOG_HPP
#define A2GUI_LOGINDIALOG_HPP

#include <memory>
#include <QtWidgets/QDialog>
#include <QtWidgets/QWidget>

#include "andromeda/Utilities.hpp"

class BackendManager;
namespace Andromeda { class Backend; }
namespace Ui { class LoginDialog; }

class LoginDialog : public QDialog
{
    Q_OBJECT

public:

    explicit LoginDialog(QWidget& parent, BackendManager& backendManager);

    virtual ~LoginDialog();

    Andromeda::Backend* GetBackend() { return mBackend; }

public slots:

    void accept() override;

private:

    BackendManager& mBackendManager;

    Andromeda::Backend* mBackend { nullptr };

    std::unique_ptr<Ui::LoginDialog> mQtUi;

    Andromeda::Debug mDebug;
};

#endif // LOGINDIALOG_HPP
