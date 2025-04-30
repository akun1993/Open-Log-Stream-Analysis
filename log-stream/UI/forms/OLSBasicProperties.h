#ifndef OLSBASICPROPERTIES_H
#define OLSBASICPROPERTIES_H

#include <QDialog>

namespace Ui {
class OLSBasicProperties;
}

class OLSBasicProperties : public QDialog
{
    Q_OBJECT

public:
    explicit OLSBasicProperties(QWidget *parent = nullptr);
    ~OLSBasicProperties();

private:
    Ui::OLSBasicProperties *ui;
};

#endif // OLSBASICPROPERTIES_H
