#ifndef OLSPLAINTEXTEDIT_H
#define OLSPLAINTEXTEDIT_H

#include <QPlainTextEdit>

class OLSPlainTextEdit : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit OLSPlainTextEdit(QWidget* parent = nullptr,
        bool monospace = true);
};

#endif // OLSPLAINTEXTEDIT_H
