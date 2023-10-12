#ifndef MOUNTSDIALOG_HPP
#define MOUNTSDIALOG_HPP

#include <QtWidgets/QDialog>

#include "andromeda/common.hpp"

namespace Ui { class MountsDialog; }

namespace AndromedaGui {
namespace QtGui {

class MountsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MountsDialog(QWidget *parent = nullptr);
    
    ~MountsDialog() override;
    DELETE_COPY(MountsDialog)
    DELETE_MOVE(MountsDialog)

private:
    Ui::MountsDialog *ui;
};

} // namespace QtGui
} // namespace AndromedaGui

#endif // MOUNTSDIALOG_HPP
