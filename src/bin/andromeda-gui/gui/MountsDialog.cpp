#include "MountsDialog.hpp"
#include "ui_MountsDialog.h"

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
