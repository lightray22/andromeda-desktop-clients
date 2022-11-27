
#include <iostream>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QMessageBox>

#include "AccountTab.hpp"
#include "ui_AccountTab.h"

#include "andromeda-gui/MountManager.hpp"

#include "andromeda/Backend.hpp"
using Andromeda::Backend;
#include "andromeda-fuse/FuseAdapter.hpp"
using AndromedaFuse::FuseAdapter;

/*****************************************************/
AccountTab::AccountTab(QWidget& parent, MountManager& mountManager, Backend& backend) : QWidget(&parent),
    mBackend(backend),
    mMountManager(mountManager),
    mMountName(backend.GetName(false)),
    mQtUi(std::make_unique<Ui::AccountTab>()),
    mDebug("AccountTab")
{
    mDebug << __func__ << "()"; mDebug.Info();

    mQtUi->setupUi(this);
}

/*****************************************************/
AccountTab::~AccountTab()
{
    mDebug << __func__ << "()"; mDebug.Info();

    if (mMounted) Unmount();
}

/*****************************************************/
void AccountTab::Mount(bool autoMount)
{
    mDebug << __func__ << "()"; mDebug.Info();

    FuseAdapter::Options fuseOptions;
    fuseOptions.mountPath = mMountName;

    try
    {
        mMountManager.CreateMount(true, mBackend, fuseOptions);
    }
    catch (const FuseAdapter::Exception& ex)
    {
        std::cout << ex.what() << std::endl; 

        QMessageBox errorBox(QMessageBox::Critical, QString("Mount Error"), ex.what()); 

        errorBox.exec(); return;
    }
    
    if (!autoMount) Browse();

    mMounted = true;
    mQtUi->buttonMount->setEnabled(false);
    mQtUi->buttonUnmount->setEnabled(true);
    mQtUi->buttonBrowse->setEnabled(true);
}

/*****************************************************/
void AccountTab::Unmount()
{
    mDebug << __func__ << "()"; mDebug.Info();

    mMountManager.RemoveMount(true, mMountName);

    mMounted = false;
    mQtUi->buttonMount->setEnabled(true);
    mQtUi->buttonUnmount->setEnabled(false);
    mQtUi->buttonBrowse->setEnabled(false);
}

/*****************************************************/
void AccountTab::Browse()
{
    std::string homeRoot { mMountManager.GetHomeRoot(mMountName) };

    mDebug << __func__ << "(homeRoot: " << homeRoot << ")"; mDebug.Info();

    homeRoot.insert(0, "file:///"); QDesktopServices::openUrl(QUrl(homeRoot.c_str()));
}
