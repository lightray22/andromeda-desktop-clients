#include "MountsDialog.hpp"
#include "ui_MountsDialog.h"

namespace AndromedaGui {
namespace Gui {

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

} // namespace Gui
} // namespace AndromedaGui
