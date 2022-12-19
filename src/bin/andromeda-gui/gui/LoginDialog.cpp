#include <QtWidgets/QMessageBox>

#include "LoginDialog.hpp"
#include "ui_LoginDialog.h"

#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;
#include "andromeda-gui/BackendContext.hpp"

/*****************************************************/
LoginDialog::LoginDialog(QWidget& parent) : QDialog(&parent),
    mQtUi(std::make_unique<Ui::LoginDialog>()),
    mDebug("LoginDialog",this)
{
    mDebug << __func__ << "()"; mDebug.Info();

    mQtUi->setupUi(this);
}

/*****************************************************/
LoginDialog::~LoginDialog() 
{ 
    mDebug << __func__ << "()"; mDebug.Info();
}

/*****************************************************/
void LoginDialog::accept()
{
    mDebug << __func__ << "()"; mDebug.Info();

    std::string apiurl { mQtUi->lineEditServerUrl->text().toStdString() };
    std::string username { mQtUi->lineEditUsername->text().toStdString() };

    mDebug << __func__ << "... apiurl:(" << apiurl << ") username:(" << username << ")"; mDebug.Info();

    try
    {
        std::string password { mQtUi->lineEditPassword->text().toStdString() };
        std::string twofactor { mQtUi->lineEditTwoFactor->text().toStdString() };
        
        mBackendContext = std::make_unique<BackendContext>(apiurl, username, password, twofactor);
    }
    catch (const BaseException& ex)
    {
        mDebug << __func__ << "... " << ex.what(); mDebug.Error();

        QMessageBox::critical(this, "Login Error", ex.what()); return;
    }

    QDialog::accept();
}

/*****************************************************/
std::unique_ptr<BackendContext> LoginDialog::TakeBackend()
{
    return std::move(mBackendContext);
}
