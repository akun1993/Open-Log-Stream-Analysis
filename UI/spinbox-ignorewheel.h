#ifndef SPINBOXIGNORESCROLL_H
#define SPINBOXIGNORESCROLL_H

#include <QInputEvent>
#include <QSpinBox>
#include <QtCore/QObject>

class SpinBoxIgnoreScroll : public QSpinBox {
    Q_OBJECT

public:
    SpinBoxIgnoreScroll(QWidget* parent = nullptr);

protected:
    virtual void wheelEvent(QWheelEvent* event) override;
};

#endif // SPINBOXIGNORESCROLL_H
