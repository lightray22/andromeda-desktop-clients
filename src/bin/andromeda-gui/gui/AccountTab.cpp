
#include <iostream>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QMessageBox>

#include "AccountTab.hpp"
#include "ui_AccountTab.h"

#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;

#include "andromeda-fuse/FuseOptions.hpp"
using AndromedaFuse::FuseOptions;

#include "andromeda-gui/BackendContext.hpp"
#include "andromeda-gui/MountContext.hpp"

/*****************************************************/
AccountTab::AccountTab(QWidget& parent, std::unique_ptr<BackendContext> backendContext) : QWidget(&parent),
    mBackendContext(std::move(backendContext)),
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
}

/*****************************************************/
std::string AccountTab::GetTabName() const
{
    return mBackendContext->GetBackend().GetName(true);
}

/*****************************************************/
void AccountTab::Mount(bool autoMount)
{
    mDebug << __func__ << "()"; mDebug.Info();

    FuseOptions fuseOptions;

    BackendImpl& backend { mBackendContext->GetBackend() };
    std::string mountPath { backend.GetName(false) };
    // TODO replace / in mountPath just in case

    try
    {
        mMountContext = std::make_unique<MountContext>(
            backend, true, mountPath, fuseOptions);
    }
    catch (const BaseException& ex)
    {
        mDebug << __func__ << "... " << ex.what(); mDebug.Error();

        QMessageBox::critical(this, "Mount Error", ex.what()); return;
    }
    
    if (!autoMount) Browse();

    mQtUi->buttonMount->setEnabled(false);
    mQtUi->buttonUnmount->setEnabled(true);
    mQtUi->buttonBrowse->setEnabled(true);
}

/*****************************************************/
void AccountTab::Unmount()
{
    mDebug << __func__ << "()"; mDebug.Info();

    mMountContext.reset();

    mQtUi->buttonMount->setEnabled(true);
    mQtUi->buttonUnmount->setEnabled(false);
    mQtUi->buttonBrowse->setEnabled(false);
}

/*****************************************************/
void AccountTab::Browse()
{
    if (!mMountContext) return; // check nullptr

    std::string homeRoot { mMountContext->GetMountPath() };

    mDebug << __func__ << "(homeRoot: " << homeRoot << ")"; mDebug.Info();

    homeRoot.insert(0, "file:///"); QDesktopServices::openUrl(QUrl(homeRoot.c_str()));
}
