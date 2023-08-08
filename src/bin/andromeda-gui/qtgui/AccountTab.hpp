#ifndef A2GUI_ACCOUNTTAB_HPP
#define A2GUI_ACCOUNTTAB_HPP

#include <memory>
#include <QtWidgets/QWidget>

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"

namespace AndromedaGui {

class BackendContext;
class MountContext;

namespace QtGui {

namespace Ui { class AccountTab; }

/** The main tab widget for an account/backend instance */
class AccountTab : public QWidget
{
    Q_OBJECT

public:

    /** 
     * Creates a new AccountTab
     * @param backendContext unique_ptr to backend (takes ownership)
     */
    explicit AccountTab(QWidget& parent, std::unique_ptr<BackendContext> backendContext);
    
    ~AccountTab() override;
    DELETE_COPY(AccountTab)
    DELETE_MOVE(AccountTab)

    /** Returns the string to be used for this tab's title */
    std::string GetTabName() const;

    /** Returns a reference to the BackendContext in use */
    inline BackendContext& GetBackendContext() const { return *mBackendContext; }

public slots:

    /** 
     * GUI action to mount this account's files 
     * @param autoMount if false, user requested -> also Browse()
     */
    void Mount(bool autoMount = false);

    /** GUI action to unmount this account's files */
    void Unmount();

    /** GUI action to browse this account's files */
    void Browse();

private:

    /** The BackendContext associated with this account (never null) */
    std::unique_ptr<BackendContext> mBackendContext;
    /** The MountContext associated with this account, if mounted */
    std::unique_ptr<MountContext> mMountContext;

    std::unique_ptr<Ui::AccountTab> mQtUi;

    mutable Andromeda::Debug mDebug;
};

} // namespace QtGui
} // namespace AndromedaGui

#endif // A2GUI_ACCOUNTTAB_HPP
