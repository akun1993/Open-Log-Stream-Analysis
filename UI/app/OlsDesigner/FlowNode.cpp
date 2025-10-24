#include "FlowNode.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QToolTip>
#include <QGraphicsView>
#include <QGraphicsSceneHoverEvent>
#include <QMenu>
#include "FlowEdge.h"
#include <QDebug>

FlowNode::FlowNode(const QString &id, const QString &label, QMenu *contextMenu,QGraphicsItem *parent)
    : QGraphicsRectItem(parent), m_id(id), m_label(label),myContextMenu(contextMenu)
{

    setRect(0, 0, NODE_WIDTH, NODE_HEIGHT);


    m_normalBrush = QBrush(QColor(120, 120, 120));
    m_activatedBrush = QBrush(QColor(65, 105, 225));
    m_activeBrush = QBrush(QColor(50, 200, 50));
    m_disabledBrush = QBrush(QColor(100, 100, 100));
    m_errorBrush = QBrush(QColor(220, 20, 60));


    m_activatedPen = QPen(QColor(200, 230, 255), 2);
    m_activePen = QPen(QColor(144, 238, 144), 3);
    m_waitingPen = QPen(QColor(255, 235, 100), 2);
    m_disabledPen = QPen(QColor(105, 105, 105), 1, Qt::DashLine);
    m_errorPen = QPen(QColor(255, 99, 71), 3);


    m_font = QFont("Microsoft YaHei", 18, QFont::Medium);
    setFlags(ItemIsSelectable | ItemIsMovable);
    setFlag(ItemSendsGeometryChanges);

    setAcceptHoverEvents(true);
}

//void FlowNode::setState(NodeState state)
//{
//    if (state != DisabledState && state != NormalState)
//    {
//        m_hasActivated = true;
//    }

//    m_state = state;
//    update();
//}

void FlowNode::setPosition(const QPointF &pos)
{
    setPos(pos);
}

void FlowNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);


    QBrush brush;
    QPen pen;

    switch (m_state)
    {
//    case ActiveState:
//        brush = m_activeBrush;
//        pen = m_activePen;
//        break;
//    case DisabledState:
//        brush = m_disabledBrush;
//        pen = m_disabledPen;
//        break;
//    case ErrorState:
//        brush = m_errorBrush;
//        pen = m_errorPen;
//        break;
    default:
        if (m_hasActivated)
        {
            brush = m_activatedBrush;
            pen = m_activatedPen;
        }
        else
        {
            brush = m_disabledBrush;
            pen = m_disabledPen;
        }
        break;
    }



    QRectF rect = boundingRect();
    QPainterPath path;
    path.addRoundedRect(rect, 15, 15);


    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    QColor baseColor = brush.color();
    gradient.setColorAt(0, baseColor.lighter(120));
    gradient.setColorAt(0.5, baseColor);
    gradient.setColorAt(1, baseColor.darker(120));

    painter->setBrush(gradient);
    painter->setPen(pen);
    painter->drawPath(path);


    painter->setFont(m_font);
    QRectF textRect = rect.adjusted(5, 5, -5, -5);


    QColor textColor = (baseColor.lightness() > 150) ? Qt::black : Qt::white;
    painter->setPen(textColor);


    QString elidedText = painter->fontMetrics().elidedText(
        m_label, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, elidedText);


    if (isSelected())
    {
        painter->setPen(QPen(Qt::yellow, 2, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(rect.adjusted(2, 2, -2, -2), 13, 13);
    }
}

void FlowNode::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{

    if (!m_label.isEmpty())
    {
        QPoint globalPos = scene()->views().first()->mapToGlobal(
            scene()->views().first()->mapFromScene(event->scenePos()));
        QToolTip::showText(globalPos, m_label);
    }


    QGraphicsRectItem::hoverEnterEvent(event);
}

void FlowNode::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{

    QToolTip::hideText();


    QGraphicsRectItem::hoverLeaveEvent(event);
}


QVariant FlowNode::itemChange(GraphicsItemChange change, const QVariant &value)
{
    switch (change) {
    case ItemPositionHasChanged:

        for (FlowEdge *arrow : qAsConst(arrows))
            arrow->updatePath();
        break;
    default:
        break;
    };

    return QGraphicsItem::itemChange(change, value);
}


//! [1]
void FlowNode::removeArrow(FlowEdge *arrow)
{
    arrows.removeAll(arrow);
}
//! [1]

//! [2]
void FlowNode::removeArrows()
{
    // need a copy here since removeArrow() will
    // modify the arrows container
    const auto arrowsCopy = arrows;
    for (FlowEdge *arrow : arrowsCopy) {
        arrow->sourceNode()->removeArrow(arrow);
        arrow->targetNode()->removeArrow(arrow);
        scene()->removeItem(arrow);
        delete arrow;
    }
}
//! [2]

//! [3]
void FlowNode::addArrow(FlowEdge *arrow)
{
    arrows.append(arrow);
}
//! [3]

//! [4]
QPixmap FlowNode::image() const
{
    QPixmap pixmap(250, 250);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(QPen(Qt::black, 8));
    painter.translate(125, 125);
   // painter.drawPolyline(myPolygon);

    return pixmap;
}
//! [4]

//! [5]
void FlowNode::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    scene()->clearSelection();
    setSelected(true);
    myContextMenu->exec(event->screenPos());
}
//! [5]

//! [6]
//QVariant FlowNode::itemChange(GraphicsItemChange change, const QVariant &value)
//{
//    if (change == QGraphicsItem::ItemPositionChange) {
//        for (FlowEdge *arrow : qAsConst(arrows))
//            arrow->updatePath();
//    }

//    return value;
//}
//! [6]


//! [12]
// void FlowNode::mousePressEvent(QGraphicsSceneMouseEvent *event)
// {
//     update();
//     QGraphicsItem::mousePressEvent(event);
// }

// void FlowNode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
// {
//     update();
//     QGraphicsItem::mouseReleaseEvent(event);
// }
