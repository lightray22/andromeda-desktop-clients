#include <QtWidgets/QMessageBox>

#include "LoginDialog.hpp"
#include "ui_LoginDialog.h"

#include "andromeda/Utilities.hpp"
using Andromeda::Utilities;
#include "andromeda-gui/BackendContext.hpp"

/*****************************************************/
LoginDialog::LoginDialog(QWidget& parent) : QDialog(&parent),
    mQtUi(std::make_unique<Ui::LoginDialog>()),
    mDebug("LoginDialog")
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

    std::string protocol { mQtUi->comboBoxProtocol->currentText().toStdString() };
    std::string apiurl { protocol + mQtUi->lineEditServerUrl->text().toStdString() };
    std::string username { mQtUi->lineEditUsername->text().toStdString() };

    mDebug << __func__ << "... apiurl:(" << apiurl << ") username:(" << username << ")"; mDebug.Info();

    try
    {
        std::string password { mQtUi->lineEditPassword->text().toStdString() };
        std::string twofactor { mQtUi->lineEditTwoFactor->text().toStdString() };
        
        mBackendContext = std::make_unique<BackendContext>(apiurl, username, password, twofactor);
    }
    catch (const Utilities::Exception& ex)
    {
        std::cout << ex.what() << std::endl; 

        // TODO make this a reusable class that only needs text
        QMessageBox errorBox(QMessageBox::Critical, QString("Login Error"), ex.what()); 

        errorBox.exec(); return;
    }

    QDialog::accept();
}

/*****************************************************/
std::unique_ptr<BackendContext> LoginDialog::TakeBackend()
{
    return std::move(mBackendContext);
}
