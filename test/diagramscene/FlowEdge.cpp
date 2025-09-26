// /****************************************************************************
// **
// ** Copyright (C) 2016 The Qt Company Ltd.
// ** Contact: https://www.qt.io/licensing/
// **
// ** This file is part of the examples of the Qt Toolkit.
// **
// ** $QT_BEGIN_LICENSE:BSD$
// ** Commercial License Usage
// ** Licensees holding valid commercial Qt licenses may use this file in
// ** accordance with the commercial license agreement provided with the
// ** Software or, alternatively, in accordance with the terms contained in
// ** a written agreement between you and The Qt Company. For licensing terms
// ** and conditions see https://www.qt.io/terms-conditions. For further
// ** information use the contact form at https://www.qt.io/contact-us.
// **
// ** BSD License Usage
// ** Alternatively, you may use this file under the terms of the BSD license
// ** as follows:
// **
// ** "Redistribution and use in source and binary forms, with or without
// ** modification, are permitted provided that the following conditions are
// ** met:
// **   * Redistributions of source code must retain the above copyright
// **     notice, this list of conditions and the following disclaimer.
// **   * Redistributions in binary form must reproduce the above copyright
// **     notice, this list of conditions and the following disclaimer in
// **     the documentation and/or other materials provided with the
// **     distribution.
// **   * Neither the name of The Qt Company Ltd nor the names of its
// **     contributors may be used to endorse or promote products derived
// **     from this software without specific prior written permission.
// **
// **
// ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// ** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// ** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// ** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// ** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// ** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// ** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// ** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
// **
// ** $QT_END_LICENSE$
// **
// ****************************************************************************/


// #include "arrow.h"
// #include "diagramitem.h"

// #include <QPainter>
// #include <QPen>
// #include <QtMath>

// //! [0]
// Arrow::Arrow(DiagramItem *startItem, DiagramItem *endItem, QGraphicsItem *parent)
//     : QGraphicsLineItem(parent), myStartItem(startItem), myEndItem(endItem)
// {
//     setFlag(QGraphicsItem::ItemIsSelectable, true);
//     setPen(QPen(myColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
// }
// //! [0]

// //! [1]
// QRectF Arrow::boundingRect() const
// {
//     qreal extra = (pen().width() + 20) / 2.0;

//     return QRectF(line().p1(), QSizeF(line().p2().x() - line().p1().x(),
//                                       line().p2().y() - line().p1().y()))
//         .normalized()
//         .adjusted(-extra, -extra, extra, extra);
// }
// //! [1]

// //! [2]
// QPainterPath Arrow::shape() const
// {
//     QPainterPath path = QGraphicsLineItem::shape();
//     path.addPolygon(arrowHead);
//     return path;
// }
// //! [2]

// //! [3]
// void Arrow::updatePosition()
// {
//     QLineF line(mapFromItem(myStartItem, 0, 0), mapFromItem(myEndItem, 0, 0));
//     setLine(line);
// }
// //! [3]

// //! [4]
// void Arrow::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
//                   QWidget *)
// {
//     if (myStartItem->collidesWithItem(myEndItem))
//         return;

//     QPen myPen = pen();
//     myPen.setColor(myColor);
//     qreal arrowSize = 20;
//     painter->setPen(myPen);
//     painter->setBrush(myColor);
// //! [4] //! [5]

//     QLineF centerLine(myStartItem->pos(), myEndItem->pos());
//     QPolygonF endPolygon = myEndItem->polygon();
//     QPointF p1 = endPolygon.first() + myEndItem->pos();
//     QPointF intersectPoint;
//     for (int i = 1; i < endPolygon.count(); ++i) {
//         QPointF p2 = endPolygon.at(i) + myEndItem->pos();
//         QLineF polyLine = QLineF(p1, p2);
//         QLineF::IntersectionType intersectionType =
//             polyLine.intersects(centerLine, &intersectPoint);
//         if (intersectionType == QLineF::BoundedIntersection)
//             break;
//         p1 = p2;
//     }

//     setLine(QLineF(intersectPoint, myStartItem->pos()));
// //! [5] //! [6]

//     double angle = std::atan2(-line().dy(), line().dx());

//     QPointF arrowP1 = line().p1() + QPointF(sin(angle + M_PI / 3) * arrowSize,
//                                     cos(angle + M_PI / 3) * arrowSize);
//     QPointF arrowP2 = line().p1() + QPointF(sin(angle + M_PI - M_PI / 3) * arrowSize,
//                                     cos(angle + M_PI - M_PI / 3) * arrowSize);

//     arrowHead.clear();
//     arrowHead << line().p1() << arrowP1 << arrowP2;
// //! [6] //! [7]
//     painter->drawLine(line());
//     painter->drawPolygon(arrowHead);
//     if (isSelected()) {
//         painter->setPen(QPen(myColor, 1, Qt::DashLine));
//         QLineF myLine = line();
//         myLine.translate(0, 4.0);
//         painter->drawLine(myLine);
//         myLine.translate(0,-8.0);
//         painter->drawLine(myLine);
//     }
// }
// //! [7]


#include "FlowEdge.h"
#include "FlowChartScene.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QPen>
#include <QPainterPath>
#include <QDebug>
#include <cmath>

FlowEdge::FlowEdge(FlowNode *source, FlowNode *target, const QString &label,
                   QGraphicsItem *parent)
    : QGraphicsPathItem(parent),
    m_source(source),
    m_target(target),
    m_label(label),
  //  m_state(NormalState),
    m_hasActivated(false),
    m_sourceChannelIndex(0),
    m_sourceTotalChannels(1),
    m_targetChannelIndex(0),
    m_targetTotalChannels(1),
    m_sourceDot(new QGraphicsEllipseItem(this))
{
    // 原点样式初始化
    m_sourceDot->setRect(-DOT_RADIUS, -DOT_RADIUS, DOT_RADIUS * 2, DOT_RADIUS * 2);
    m_sourceDot->setBrush(Qt::white);
    m_sourceDot->setPen(Qt::NoPen);
    m_sourceDot->setZValue(3);

    // 画笔样式

    m_activatedPen = QPen(QColor(100, 149, 237), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    m_activePen = QPen(QColor(50, 200, 50), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    m_disabledPen = QPen(QColor(150, 150, 150), 1, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
    m_errorPen = QPen(QColor(255, 50, 50), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    setPen(m_activePen);
}

void FlowEdge::updatePath()
{
    if (!m_source || !m_target)
        return;

    drawOrthogonalPath();
    updateSourceDotPosition();
}

void FlowEdge::drawOrthogonalPath()
{
    if (!m_source || !m_target)
        return;

    QRectF sourceRect = m_source->sceneBoundingRect();
    QRectF targetRect = m_target->sceneBoundingRect();

    // 源节点出边偏移：确保同一源节点的出边分散
    const double sourceBaseSpacing = 10.0;
    double sourceVerticalOffset = 0;
    if (m_sourceTotalChannels > 1)
    {
        double sourceMiddle = (m_sourceTotalChannels - 1) / 2.0;
        sourceVerticalOffset = (m_sourceChannelIndex - sourceMiddle) * sourceBaseSpacing;
    }

    // 目标节点入边偏移：确保同一目标节点的入边分散
    const double targetBaseSpacing = 10.0;

    // 动态调整间距：入边越多间距越大（避免拥挤）
    double targetSpacing = targetBaseSpacing * (1 + m_targetTotalChannels / 5.0);
    double targetVerticalOffset = 0;
    if (m_targetTotalChannels > 1)
    {
        double targetMiddle = (m_targetTotalChannels - 1) / 2.0;
        targetVerticalOffset = (m_targetChannelIndex - targetMiddle) * targetSpacing;
    }

    // 2. 计算起点和终点（带双向偏移）
    QPointF startPoint(
        sourceRect.right(),
        sourceRect.center().y() + sourceVerticalOffset // 源节点偏移
        );

    QPointF endPoint(
        targetRect.left(),
        targetRect.center().y() + targetVerticalOffset // 目标节点偏移（解决入边重叠）
        );

    // 设置当前画笔
    QPen currentPen;
    // switch (m_state)
    // {
    // case ActiveState:
    //     currentPen = m_activePen;
    //     break;
    //     break;
    // case DisabledState:
    //     currentPen = m_disabledPen;
    //     break;
    // case ErrorState:
    //     currentPen = m_errorPen;
    //     break;
    // default:
    //     currentPen = m_hasActivated ? m_activatedPen : m_disabledPen;
    //     break;
    // }

    currentPen = m_activatedPen;
    setPen(currentPen);

    // 获取场景中所有节点（用于避障）
    QList<FlowNode *> allNodes;
    for (QGraphicsItem *item : scene()->items())
    {
        if (FlowNode *node = dynamic_cast<FlowNode *>(item))
        {
            allNodes.append(node);
        }
    }

    // 计算场景边界和中心
    QRectF sceneRect = scene()->sceneRect();
    QPointF sceneCenter = sceneRect.center();

    // 计算直接距离
    double distance = QLineF(startPoint, endPoint).length();

    QPainterPath path;
    path.moveTo(startPoint);

    if (distance > 1200) // 长距离路径
    {
        // 确定最佳绕行方向
        double verticalOffset = calculateBestVerticalOffset(
            startPoint, endPoint, sceneCenter, allNodes);

        // 计算控制点
        double horizontalOffset = qMin(distance / 3.0, 300.0);

        // 创建分段路径
        QPointF controlPoint1(startPoint.x() + horizontalOffset,
                              startPoint.y() + verticalOffset);

        QPointF controlPoint2(endPoint.x() - horizontalOffset,
                              endPoint.y() + verticalOffset);

        //  使用三次贝塞尔曲线创建平滑路径
        path.cubicTo(controlPoint1, controlPoint2, endPoint);

        //  检查并修正路径冲突
        if (pathCollidesWithNodes(path, allNodes))
        {
            // 如果检测到冲突，增加偏移量重新计算
            verticalOffset *= 1.8; // 显著增加偏移量
            controlPoint1 = QPointF(startPoint.x() + horizontalOffset,
                                    startPoint.y() + verticalOffset);
            controlPoint2 = QPointF(endPoint.x() - horizontalOffset,
                                    endPoint.y() + verticalOffset);

            path = QPainterPath();
            path.moveTo(startPoint);
            path.cubicTo(controlPoint1, controlPoint2, endPoint);
        }
    }
    else if (distance > 500)
    {
        const double maxVerticalOffset = 150.0; // 最大垂直偏移
        const double minVerticalOffset = 50.0;  // 最小垂直偏移

        // 动态计算偏移量（距离越长偏移越大）
        double ratio = (distance - 500) / (1200 - 500);
        double verticalOffset = minVerticalOffset +
                                ratio * (maxVerticalOffset - minVerticalOffset);

        // // 应用方向调整
        // verticalOffset = calculateDirectionalOffset(startPoint, endPoint, verticalOffset);
        // 确定最佳绕行方向
        verticalOffset = calculateBestVerticalOffset(
            startPoint, endPoint, sceneCenter, allNodes);
        verticalOffset /= 2;
        // 计算控制点（更紧凑的曲线）
        double horizontalOffset = qMin(distance / 4.0, 200.0);

        QPointF controlPoint1(startPoint.x() + horizontalOffset,
                              startPoint.y() + verticalOffset);

        QPointF controlPoint2(endPoint.x() - horizontalOffset,
                              endPoint.y() + verticalOffset);

        // 绘制曲线
        path.cubicTo(controlPoint1, controlPoint2, endPoint);
    }
    else // 短距离路径（正交路由+中间点偏移）
    {
        // 中间点X坐标，根据目标入边索引微调（避免重叠）
        double midX = (startPoint.x() + endPoint.x()) / 2.0;
        midX += (m_targetChannelIndex - (m_targetTotalChannels - 1) / 2.0) * 20; // 横向微

        QPointF controlPoint1(midX, startPoint.y());
        QPointF controlPoint2(midX, endPoint.y());
        path.cubicTo(controlPoint1, controlPoint2, endPoint);
    }

    //  添加箭头
    QPointF arrowTip = endPoint;
    QPointF arrowDir = path.pointAtPercent(0.99) - path.pointAtPercent(0.98);
    double length = arrowDir.manhattanLength();
    if (length > 0)
    {
        arrowDir = arrowDir / length * 10;
        QPointF perpendicular(-arrowDir.y(), arrowDir.x());

        QPainterPath arrow;
        arrow.moveTo(arrowTip);
        arrow.lineTo(arrowTip - arrowDir + perpendicular * 0.5);
        arrow.lineTo(arrowTip - arrowDir - perpendicular * 0.5);
        arrow.closeSubpath();
        path.addPath(arrow);
    }

    setPath(path);

    // 添加标签（带背景）
    if (!m_label.isEmpty())
    {
        //        QPointF labelPos = path.pointAtPercent(0.5);
        //        QGraphicsSimpleTextItem *labelItem = new QGraphicsSimpleTextItem(m_label, this);
        //        labelItem->setFont(QFont("Arial", 12, QFont::Bold));

        //        QGraphicsRectItem *bgItem = new QGraphicsRectItem(this);
        //        QRectF textRect = labelItem->boundingRect();
        //        bgItem->setRect(textRect.adjusted(-2, -1, 2, 1));
        //        bgItem->setPos(labelPos - QPointF(textRect.width() / 2, textRect.height() / 2));
        //        bgItem->setBrush(QBrush(QColor(255, 255, 255, 200)));
        //        bgItem->setPen(Qt::NoPen);
        //        bgItem->setZValue(1);

        //        QColor textColor = m_hasActivated ? QColor(78, 155, 121) : Qt::darkGray;
        //        labelItem->setBrush(textColor);
        //        labelItem->setPos(labelPos - QPointF(textRect.width() / 2, textRect.height() / 2));
        //        labelItem->setZValue(2);
    }
}

double FlowEdge::calculateDirectionalOffset(const QPointF &start, const QPointF &end, double baseOffset)
{
    // 计算线段角度（0-180度）
    QLineF line(start, end);
    double angle = line.angle();

    // 标准化角度到0-180范围
    if (angle > 180)
        angle = 360 - angle;

    // 根据角度调整偏移方向
    if (angle < 30)
    {
        return (start.y() < end.y()) ? baseOffset : -baseOffset;
    }
    else if (angle > 150)
    { // 接近垂直
        return (start.x() < end.x()) ? baseOffset : -baseOffset;
    }
    else
    { // 对角线
        return (start.y() < end.y()) ? baseOffset : -baseOffset;
    }
}

void FlowEdge::updateSourceDotPosition()
{
    if (!m_source)
        return;

    QRectF sourceRect = m_source->sceneBoundingRect();
    const double sourceBaseSpacing = 10.0;

    // 源节点出边偏移（与路径保持一致）
    double sourceVerticalOffset = 0;
    if (m_sourceTotalChannels > 1)
    {
        double sourceMiddle = (m_sourceTotalChannels - 1) / 2.0;
        sourceVerticalOffset = (m_sourceChannelIndex - sourceMiddle) * sourceBaseSpacing;
    }

    QPointF dotPos(sourceRect.right(), sourceRect.center().y() + sourceVerticalOffset);
    m_sourceDot->setPos(dotPos);
}

// void FlowEdge::setState(NodeState state)
// {
//     if (state != DisabledState && state != NormalState)
//         m_hasActivated = true;

//     m_state = state;
// }

double FlowEdge::calculateBestVerticalOffset(const QPointF &start, const QPointF &end, const QPointF &sceneCenter, const QList<FlowNode *> &allNodes)
{
    // 分析场景区域节点密度
    double topDensity = calculateRegionDensity(
        QRectF(sceneCenter.x() - 200, 0, 400, sceneCenter.y()),
        allNodes);

    double bottomDensity = calculateRegionDensity(
        QRectF(sceneCenter.x() - 200, sceneCenter.y(), 400, sceneCenter.y()),
        allNodes);

    //  基于密度决定偏移方向
    double baseOffset = 250.0; // 基本偏移量

    // 优先选择密度较小的区域
    if (topDensity < bottomDensity)
    {
        // 上方密度小，向上偏移
        baseOffset = -baseOffset;
    }

    // 3. 根据连接线位置微调
    double positionFactor = 1.0;
    if (start.y() > sceneCenter.y() && end.y() > sceneCenter.y()) //
    {
        // 都在上方，增加向下偏移
        positionFactor = 1.1;
    }
    else if (start.y() < sceneCenter.y() && end.y() < sceneCenter.y()) //
    {
        // 都在下方，增加向上偏移
        positionFactor = -1.1;
    }

    // 添加通道索引偏移（避免重叠）
    double channelOffset = (m_targetChannelIndex - (m_targetTotalChannels - 1) / 2.0) * 50;

    return baseOffset * positionFactor + channelOffset;
}

double FlowEdge::calculateRegionDensity(
    const QRectF &region,
    const QList<FlowNode *> &allNodes)
{
    int nodeCount = 0;
    double totalArea = region.width() * region.height();

    if (totalArea <= 0)
        return 0.0;

    for (FlowNode *node : allNodes)
    {
        if (region.intersects(node->sceneBoundingRect()))
        {
            nodeCount++;
        }
    }

    return nodeCount / totalArea;
}

bool FlowEdge::pathCollidesWithNodes(
    const QPainterPath &path,
    const QList<FlowNode *> &allNodes)
{
    for (FlowNode *node : allNodes)
    {
        if (node == m_source || node == m_target)
            continue;

        if (path.intersects(node->sceneBoundingRect()))
        {
            return true;
        }
    }
    return false;
}

