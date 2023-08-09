
#include <iostream>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QMessageBox>

#include "AccountTab.hpp"
#include "Utilities.hpp"
#include "ui_AccountTab.h"

#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;

#include "andromeda-fuse/FuseOptions.hpp"
using AndromedaFuse::FuseOptions;

#include "andromeda-gui/BackendContext.hpp"
#include "andromeda-gui/MountContext.hpp"

namespace AndromedaGui {
namespace QtGui {

/*****************************************************/
AccountTab::AccountTab(QWidget& parent, std::unique_ptr<BackendContext> backendContext) : QWidget(&parent),
    mBackendContext(std::move(backendContext)),
    mQtUi(std::make_unique<Ui::AccountTab>()),
    mDebug(__func__,this)
{
    MDBG_INFO("()");

    mQtUi->setupUi(this);
}

/*****************************************************/
AccountTab::~AccountTab()
{
    MDBG_INFO("()");
}

/*****************************************************/
std::string AccountTab::GetTabName() const
{
    return mBackendContext->GetBackend().GetName(true);
}

/*****************************************************/
void AccountTab::Mount(bool autoMount)
{
    MDBG_INFO("()");

    FuseOptions fuseOptions;

    BackendImpl& backend { mBackendContext->GetBackend() };
    const std::string mountPath { backend.GetName(false) };

    try
    {
        mMountContext = std::make_unique<MountContext>(
            backend, true, mountPath, fuseOptions);
    }
    catch (const BaseException& ex)
    {
        MDBG_ERROR("... " << ex.what());
        Utilities::criticalBox(this, "Mount Error", "Failed to create filesystem mount.", ex);
        return; // do not continue
    }
    
    if (!autoMount) Browse();

    mQtUi->buttonMount->setEnabled(false);
    mQtUi->buttonUnmount->setEnabled(true);
    mQtUi->buttonBrowse->setEnabled(true);
}

/*****************************************************/
void AccountTab::Unmount()
{
    MDBG_INFO("()");

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

    MDBG_INFO("(homeRoot: " << homeRoot << ")");

    homeRoot.insert(0, "file:///"); QDesktopServices::openUrl(QUrl(homeRoot.c_str()));
}

} // namespace QtGui
} // namespace AndromedaGui
