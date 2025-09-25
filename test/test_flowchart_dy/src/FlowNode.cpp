#include "FlowNode.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QToolTip>
#include <QGraphicsView>
#include <QGraphicsSceneHoverEvent>

FlowNode::FlowNode(const QString &id, const QString &label,
                   QGraphicsItem *parent)
    : QGraphicsRectItem(parent), m_id(id), m_label(label)
{
    // 设置大小
    setRect(0, 0, NODE_WIDTH, NODE_HEIGHT);

    // 更新颜色方案 - 增强对比度
    m_normalBrush = QBrush(QColor(120, 120, 120));   // 未激活 - 深灰色
    m_activatedBrush = QBrush(QColor(65, 105, 225)); // 已激活 - 皇家蓝（更鲜艳）
    m_activeBrush = QBrush(QColor(50, 200, 50));     // 运行中 - 亮绿色
    m_disabledBrush = QBrush(QColor(100, 100, 100)); // 禁用 - 灰色
    m_errorBrush = QBrush(QColor(220, 20, 60));      // 错误 - 猩红色
    m_skipBrush = QBrush(QColor(148, 0, 211));       // 紫色 - 跳过状态
    m_skipPen = QPen(QColor(200, 200, 255), 2);      // 浅紫色边框

    m_activatedPen = QPen(QColor(200, 230, 255), 2); // 已激活边框
    m_activePen = QPen(QColor(144, 238, 144), 3);    // 运行中边框
    m_waitingPen = QPen(QColor(255, 235, 100), 2);   // 等待中边框
    m_disabledPen = QPen(QColor(105, 105, 105), 1, Qt::DashLine);
    m_errorPen = QPen(QColor(255, 99, 71), 3); // 错误边框

    // 设置字体
    m_font = QFont("Microsoft YaHei", 18, QFont::Medium);
    setFlags(ItemIsSelectable | ItemIsMovable);
    // 启用鼠标悬停事件
    setAcceptHoverEvents(true);
}

void FlowNode::setState(NodeState state)
{
    // 当状态变为非禁用状态时标记为已激活
    if (state != DisabledState && state != NormalState)
    {
        m_hasActivated = true;
    }

    m_state = state;
    update();
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

    // 根据状态和激活状态选择画笔和画刷
    QBrush brush;
    QPen pen;
    if (m_isSkip)
    {
        brush = m_skipBrush;
        pen = m_skipPen;
    }
    else
    {
        switch (m_state)
        {
        case ActiveState:
            brush = m_activeBrush;
            pen = m_activePen;
            break;
        case DisabledState:
            brush = m_disabledBrush;
            pen = m_disabledPen;
            break;
        case ErrorState:
            brush = m_errorBrush;
            pen = m_errorPen;
            break;
        default:
            if (m_hasActivated)
            {
                brush = m_activatedBrush; // 已激活状态
                pen = m_activatedPen;
            }
            else
            {
                brush = m_disabledBrush; // 未激活状态
                pen = m_disabledPen;
            }
            break;
        }
    }

    // 绘制圆角矩形
    QRectF rect = boundingRect();
    QPainterPath path;
    path.addRoundedRect(rect, 15, 15);

    // 添加渐变效果
    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    QColor baseColor = brush.color();
    gradient.setColorAt(0, baseColor.lighter(120));
    gradient.setColorAt(0.5, baseColor);
    gradient.setColorAt(1, baseColor.darker(120));

    painter->setBrush(gradient);
    painter->setPen(pen);
    painter->drawPath(path);

    // 绘制文本
    painter->setFont(m_font);
    QRectF textRect = rect.adjusted(5, 5, -5, -5);

    // 文本颜色根据背景亮度自动调整
    QColor textColor = (baseColor.lightness() > 150) ? Qt::black : Qt::white;
    painter->setPen(textColor);

    // 文本绘制优化
    QString elidedText = painter->fontMetrics().elidedText(
        m_label, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, elidedText);

    // 选中状态高亮
    if (isSelected())
    {
        painter->setPen(QPen(Qt::yellow, 2, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(rect.adjusted(2, 2, -2, -2), 13, 13);
    }
}

void FlowNode::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    // 显示提示文本
    if (!m_label.isEmpty())
    {
        QPoint globalPos = scene()->views().first()->mapToGlobal(
            scene()->views().first()->mapFromScene(event->scenePos()));
        QToolTip::showText(globalPos, m_label);
    }

    // 调用基类处理
    QGraphicsRectItem::hoverEnterEvent(event);
}

void FlowNode::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    // 隐藏提示文本
    QToolTip::hideText();

    // 调用基类处理
    QGraphicsRectItem::hoverLeaveEvent(event);
}


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