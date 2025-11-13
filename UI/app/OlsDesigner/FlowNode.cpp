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
#include "properties-view.h"
#include <QDebug>
#include <string>
#include <QAction>

FlowNode::FlowNode(PluginType nodeType, int id,const QString &label, QMenu *contextMenu,QGraphicsItem *parent)
    : QGraphicsRectItem(parent), m_pluginType(nodeType), m_id(id), m_label(label),myContextMenu(contextMenu)
{

    setRect(0, 0, NODE_WIDTH, NODE_HEIGHT);


    m_inactiveBrush = QBrush(QColor(120, 120, 120));

    m_activeBrush = QBrush(QColor(50, 200, 50));

    m_inputBrush = QBrush(QColor(152,251,152));
    m_processBrush = QBrush(QColor(255,165,0));
    m_outputBrush = QBrush(QColor(135,206,235));


    m_inputPen = QPen(QColor(255, 99, 71 ), 2);
    m_processPen = QPen(QColor(255, 235, 100), 2);
    m_outputPen = QPen(QColor(200, 230, 255), 3);

    m_activePen = QPen(QColor(144, 238, 144), 3);
    m_inactivePen = QPen(QColor(105, 105, 105), 1, Qt::DashLine);

    m_font = QFont("Microsoft YaHei", 28, QFont::Bold);
    setFlags(ItemIsSelectable | ItemIsMovable);
    setFlag(ItemSendsGeometryChanges);


    // qDebug("FlowNode %p\n",this);

    setAcceptHoverEvents(true);
}


void FlowNode::openPropertyView(){

   // OLSPropertiesView *view  = new OLSPropertiesView();

    qDebug("Open property view\n");
}


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

    switch (m_pluginType)
    {
    case PLUGIN_INPUT:
        brush = m_inputBrush;
        pen = m_inputPen;
        break;
    case PLUGIN_PROCESS:
        brush = m_processBrush;
        pen = m_processPen;
        break;
    case PLUGIN_OUTPUT:
        brush = m_outputBrush;
        pen = m_outputPen;
        break;
    default:
        if (m_hasActivated)
        {
            brush = m_activeBrush;
            pen = m_activePen;
        }
        else
        {
            brush = m_inactiveBrush;
            pen = m_inactivePen;
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


    QColor textColor =  Qt::black ;
    painter->setPen(textColor);

    QString elidedText = painter->fontMetrics().elidedText(
        m_label, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, elidedText);


    if (isSelected())
    {
        painter->setPen(QPen(Qt::yellow, 4, Qt::DashLine));
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


void FlowNode::removeArrow(FlowEdge *arrow)
{
    arrows.removeAll(arrow);
}

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

void FlowNode::addArrow(FlowEdge *arrow)
{
    arrows.append(arrow);
}

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



void FlowNode::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    qDebug("Open contextMenuEvent \n");
    scene()->clearSelection();
    setSelected(true);
    myContextMenu->exec(event->screenPos());
}


