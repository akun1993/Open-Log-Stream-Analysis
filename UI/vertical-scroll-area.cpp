#include "vertical-scroll-area.h"
#include <QResizeEvent>

void VScrollArea::resizeEvent(QResizeEvent* event)
{
    if (!!widget())
        widget()->setMaximumWidth(event->size().width());

    QScrollArea::resizeEvent(event);
}
