#include "spinbox-ignorewheel.h"

SpinBoxIgnoreScroll::SpinBoxIgnoreScroll(QWidget* parent)
    : QSpinBox(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

void SpinBoxIgnoreScroll::wheelEvent(QWheelEvent* event)
{
    if (!hasFocus())
        event->ignore();
    else
        QSpinBox::wheelEvent(event);
}
