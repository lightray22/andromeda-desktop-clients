#ifndef A2GUI_ACCOUNTTAB_HPP
#define A2GUI_ACCOUNTTAB_HPP

#include <memory>
#include <QtWidgets/QWidget>

#include "andromeda/Utilities.hpp"

class BackendContext;
class MountContext;
namespace Ui { class AccountTab; }

namespace Andromeda { class Backend; }

class AccountTab : public QWidget
{
    Q_OBJECT

public:

    explicit AccountTab(QWidget& parent, std::unique_ptr<BackendContext>& backendContext);
    
    virtual ~AccountTab();

public slots:

    void Mount(bool autoMount = false);

    void Unmount();

    void Browse();

private:

    std::unique_ptr<BackendContext> mBackendContext;
    std::unique_ptr<MountContext> mMountContext;

    std::unique_ptr<Ui::AccountTab> mQtUi;

    Andromeda::Debug mDebug;
};

#endif // A2GUI_ACCOUNTTAB_HPP
