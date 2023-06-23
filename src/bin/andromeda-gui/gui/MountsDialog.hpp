#ifndef MOUNTSDIALOG_HPP
#define MOUNTSDIALOG_HPP

#include <QtWidgets/QDialog>

namespace AndromedaGui {
namespace Gui {

namespace Ui { class MountsDialog; }

class MountsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MountsDialog(QWidget *parent = nullptr);
    
    ~MountsDialog() override;
    MountsDialog(const MountsDialog&) = delete; // no copy
    MountsDialog& operator=(const MountsDialog&) = delete;
    MountsDialog(MountsDialog&&) = delete; // no move
    MountsDialog& operator=(MountsDialog&&) = delete;

private:
    Ui::MountsDialog *ui;
};

} // namespace Gui
} // namespace AndromedaGui

#endif // MOUNTSDIALOG_HPP
