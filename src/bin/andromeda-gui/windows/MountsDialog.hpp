#ifndef MOUNTSDIALOG_HPP
#define MOUNTSDIALOG_HPP

#include <QtWidgets/QDialog>

namespace Ui {
class MountsDialog;
}

class MountsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MountsDialog(QWidget *parent = nullptr);
    ~MountsDialog();

private:
    Ui::MountsDialog *ui;
};

#endif // MOUNTSDIALOG_HPP
