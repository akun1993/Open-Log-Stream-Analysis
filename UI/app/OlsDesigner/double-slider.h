#ifndef DOUBLESLIDER_H
#define DOUBLESLIDER_H
#include "slider-ignorewheel.h"
#include <QSlider>

class DoubleSlider : public SliderIgnoreScroll {
    Q_OBJECT

    double minVal, maxVal, minStep;

public:
    DoubleSlider(QWidget* parent = nullptr);

    void setDoubleConstraints(double newMin, double newMax, double newStep,
        double val);

Q_SIGNALS:
    void doubleValChanged(double val);

public Q_SLOTS:
    void intValChanged(int val);
    void setDoubleVal(double val);
};
#endif // DOUBLESLIDER_H
