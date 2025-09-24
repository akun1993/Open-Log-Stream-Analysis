/********************************************************************************
** Form generated from reading UI file 'systemimagewidget.ui'
**
** Created by: Qt User Interface Compiler version 5.12.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SYSTEMIMAGEWIDGET_H
#define UI_SYSTEMIMAGEWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SystemImageWidget
{
public:
    QVBoxLayout *verticalLayout;

    void setupUi(QWidget *SystemImageWidget)
    {
        if (SystemImageWidget->objectName().isEmpty())
            SystemImageWidget->setObjectName(QString::fromUtf8("SystemImageWidget"));
        SystemImageWidget->resize(1282, 794);
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(SystemImageWidget->sizePolicy().hasHeightForWidth());
        SystemImageWidget->setSizePolicy(sizePolicy);
        SystemImageWidget->setMaximumSize(QSize(1900, 16777215));
        SystemImageWidget->setLayoutDirection(Qt::LeftToRight);
        verticalLayout = new QVBoxLayout(SystemImageWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));

        retranslateUi(SystemImageWidget);

        QMetaObject::connectSlotsByName(SystemImageWidget);
    } // setupUi

    void retranslateUi(QWidget *SystemImageWidget)
    {
        SystemImageWidget->setWindowTitle(QApplication::translate("SystemImageWidget", "Form", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SystemImageWidget: public Ui_SystemImageWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SYSTEMIMAGEWIDGET_H
