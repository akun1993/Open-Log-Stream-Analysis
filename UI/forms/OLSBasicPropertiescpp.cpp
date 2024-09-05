#include "OLSBasicProperties.h"
#include "ui_OLSBasicProperties.h"

OLSBasicProperties::OLSBasicProperties(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OLSBasicProperties)
{
    ui->setupUi(this);
}

OLSBasicProperties::~OLSBasicProperties()
{
    delete ui;
}
