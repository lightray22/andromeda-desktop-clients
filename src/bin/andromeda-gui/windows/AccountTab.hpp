#ifndef A2GUI_ACCOUNTTAB_HPP
#define A2GUI_ACCOUNTTAB_HPP

#include <memory>
#include <QtWidgets/QWidget>

#include "andromeda/Utilities.hpp"

class MountManager;
namespace Andromeda { class Backend; }
namespace Ui { class AccountTab; }

class AccountTab : public QWidget
{
    Q_OBJECT

public:

    explicit AccountTab(QWidget& parent, MountManager& mountManager, Andromeda::Backend& backend);
    
    virtual ~AccountTab();

    Andromeda::Backend& GetBackend(){ return mBackend; }

public slots:

    void Mount(bool autoMount = false);

    void Unmount();

    void Browse();

private:

    Andromeda::Backend& mBackend;
    MountManager& mMountManager;

    bool mMounted { false };
    std::string mMountName;

    std::unique_ptr<Ui::AccountTab> mQtUi;

    Andromeda::Debug mDebug;
};

#endif // A2GUI_ACCOUNTTAB_HPP
