#include <QtWidgets/QMessageBox>

#include "LoginDialog.hpp"
#include "ui_LoginDialog.h"

#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;
#include "andromeda-gui/BackendContext.hpp"

namespace AndromedaGui {
namespace Gui {

/*****************************************************/
LoginDialog::LoginDialog(QWidget& parent) : QDialog(&parent),
    mQtUi(std::make_unique<Ui::LoginDialog>()),
    mDebug("LoginDialog",this)
{
    MDBG_INFO("()");

    mQtUi->setupUi(this);
}

/*****************************************************/
LoginDialog::~LoginDialog() 
{ 
    MDBG_INFO("()");
}

/*****************************************************/
void LoginDialog::accept()
{
    MDBG_INFO("()");

    std::string apiurl { mQtUi->lineEditServerUrl->text().toStdString() };
    std::string username { mQtUi->lineEditUsername->text().toStdString() };

    MDBG_INFO("... apiurl:(" << apiurl << ") username:(" << username << ")");

    try
    {
        std::string password { mQtUi->lineEditPassword->text().toStdString() };
        std::string twofactor { mQtUi->lineEditTwoFactor->text().toStdString() };
        
        mBackendContext = std::make_unique<BackendContext>(apiurl, username, password, twofactor);
    }
    catch (const BaseException& ex)
    {
        MDBG_ERROR("... " << ex.what());

        QMessageBox::critical(this, "Login Error", ex.what()); return;
    }

    QDialog::accept();
}

/*****************************************************/
std::unique_ptr<BackendContext> LoginDialog::TakeBackend()
{
    return std::move(mBackendContext);
}

} // namespace Gui
} // namespace AndromedaGui
