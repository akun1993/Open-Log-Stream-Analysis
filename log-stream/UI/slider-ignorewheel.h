#ifndef SLIDERIGNORESCROLL_H
#define SLIDERIGNORESCROLL_H

#include <QAccessibleWidget>
#include <QInputEvent>
#include <QSlider>
#include <QtCore/QObject>

class SliderIgnoreScroll : public QSlider {
    Q_OBJECT

public:
    SliderIgnoreScroll(QWidget* parent = nullptr);
    SliderIgnoreScroll(Qt::Orientation orientation,
        QWidget* parent = nullptr);

protected:
    virtual void wheelEvent(QWheelEvent* event) override;
};

#endif // SLIDERIGNORESCROLL_H
