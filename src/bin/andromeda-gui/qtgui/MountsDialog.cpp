#include "MountsDialog.hpp"
#include "ui_MountsDialog.h"

namespace AndromedaGui {
namespace QtGui {

MountsDialog::MountsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MountsDialog)
{
    ui->setupUi(this);
}

MountsDialog::~MountsDialog()
{
    delete ui;
}

} // namespace QtGui
} // namespace AndromedaGui
