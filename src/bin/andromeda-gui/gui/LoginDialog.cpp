#include <QtWidgets/QMessageBox>

#include "LoginDialog.hpp"
#include "ui_LoginDialog.h"

#include "andromeda/Utilities.hpp"
using Andromeda::Utilities;
#include "andromeda-gui/BackendManager.hpp"

/*****************************************************/
LoginDialog::LoginDialog(QWidget& parent, BackendManager& backendManager) : QDialog(&parent),
    mBackendManager(backendManager),
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
        
        mBackend = &mBackendManager.AddBackend(apiurl, username, password, twofactor);
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
