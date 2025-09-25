#include "FlowChartScene.h"
#include "FlowNode.h"
#include "FlowEdge.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QGraphicsSceneMouseEvent>
#include <QTimer>
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <queue>
#include <QVector2D>
#include <QElapsedTimer>

FlowChartScene::FlowChartScene(QObject *parent)
    : QGraphicsScene(parent)
{
    // 设置渐变背景
    QLinearGradient gradient(0, 0, 0, 1);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setColorAt(1, QColor(8, 22, 64));   // #081640
    gradient.setColorAt(0, QColor(15, 76, 117)); // #0f4c75
    setBackgroundBrush(gradient);

    // 设置场景网格
    setForegroundBrush(QBrush(QColor(100, 100, 100, 20), Qt::CrossPattern));
}

void FlowChartScene::loadFlowChart(const QJsonObject &json)
{
    // 检查"开始"节点状态变化
    static NodeState lastStartState = NormalState;
    bool resetActivation = false;

    if (m_nodes.contains("begin"))
    {
        NodeState currentStartState = mapJsonStatusToNodeState(
            json["states"].toObject()["begin"].toObject()["status"].toString());
        if (lastStartState != ActiveState && currentStartState == ActiveState)
        {
            resetActivation = true;
        }
        lastStartState = currentStartState;
    }

    // 保存当前激活状态（除非需要重置）
    QMap<QString, bool> nodeActivationStates;
    QMap<QString, bool> edgeActivationStates;

    if (!resetActivation)
    {
        for (auto &node : m_nodes)
        {
            nodeActivationStates[node->id()] = node->hasActivated();
        }
        for (auto &edge : m_edges)
        {
            edgeActivationStates[edge->sourceNode()->id() + "->" + edge->targetNode()->id()] = edge->hasActivated();
        }
    }

    clearScene();
    m_jsonData = json;
    parseJson(json);
    createNodes();
    createEdges();

    // 恢复或重置激活状态
    for (auto &node : m_nodes)
    {
        if (resetActivation)
        {
            node->setHasActivated(false);
        }
        else if (nodeActivationStates.contains(node->id()))
        {
            node->setHasActivated(nodeActivationStates[node->id()]);
        }
    }

    for (auto &edge : m_edges)
    {
        QString key = edge->sourceNode()->id() + "->" + edge->targetNode()->id();
        if (resetActivation)
        {
            edge->setHasActivated(false);
        }
        else if (edgeActivationStates.contains(key))
        {
            edge->setHasActivated(edgeActivationStates[key]);
        }
    }

    // 更新激活状态
    for (auto &node : m_nodes)
    {
        if (node->state() != DisabledState && node->state() != NormalState)
        {
            node->setHasActivated(true);
        }
    }

    for (auto &edge : m_edges)
    {
        if (edge->state() != DisabledState && edge->state() != NormalState)
        {
            edge->setHasActivated(true);
        }
    }

    arrangeLayout();
}

void FlowChartScene::parseJson(const QJsonObject &json)
{
    QJsonObject states = json["states"].toObject();
    for (auto it = states.constBegin(); it != states.constEnd(); ++it)
    {
        QString nodeId = it.key();
        QJsonObject nodeData = it.value().toObject();
        QString status = nodeData["status"].toString();
        m_nodeStates[nodeId] = mapJsonStatusToNodeState(status);

        m_nodeSkips[nodeId] = nodeData.contains("is_skip") ? nodeData["is_skip"].toBool() : false;
    }
}

void FlowChartScene::clearScene()
{
    clear();
    m_nodes.clear();
    m_edges.clear();
    m_nodeLevels.clear();
    m_levelNodes.clear();
    m_maxLevel = 0;
    m_nodeStates.clear();
}

void FlowChartScene::createNodes()
{
    if (!m_jsonData.contains("states") || !m_jsonData["states"].isObject())
        return;

    QJsonObject states = m_jsonData["states"].toObject();
    for (auto it = states.constBegin(); it != states.constEnd(); ++it)
    {
        QString nodeId = it.key();
        if (m_nodes.contains(nodeId))
            continue;

        QJsonObject nodeData = it.value().toObject();
        QString label = nodeData.contains("label") ? nodeData["label"].toString() : nodeId;

        FlowNode *node = new FlowNode(nodeId, label);
        addItem(node);
        m_nodes[nodeId] = node;

        if (m_nodeSkips.contains(nodeId))
            node->setSkip(m_nodeSkips[nodeId]);
        if (m_nodeStates.contains(nodeId))
            node->setState(m_nodeStates[nodeId]);
    }
}

void FlowChartScene::createEdges()
{
    if (!m_jsonData.contains("transs") || !m_jsonData["transs"].isArray())
        return;

    QJsonArray transs = m_jsonData["transs"].toArray();
    for (const QJsonValue &value : transs)
    {
        if (!value.isObject())
            continue;

        QJsonObject edgeData = value.toObject();
        QString source = edgeData["source"].toString();
        QString target = edgeData["target"].toString();
        QString label = edgeData.contains("expr") ? edgeData["expr"].toString() : "";

        if (!m_nodes.contains(source) || !m_nodes.contains(target))
            continue;

        FlowEdge *edge = new FlowEdge(m_nodes[source], m_nodes[target], label);
        addItem(edge);
        edge->setZValue(-1);
        m_edges.append(edge);
        edge->setState(m_nodes[source]->state());
    }

    for (FlowEdge *edge : m_edges)
    {
        FlowNode *sourceNode = edge->sourceNode();
        if (sourceNode)
            sourceNode->addOutEdge(edge);
    }
}

void FlowChartScene::arrangeLayout()
{
    calculateNodeLevels();
    applySmartLayout();
    updateEdgePaths();
    createLegend();
}

void FlowChartScene::createLegend()
{
    QRectF sceneBound = sceneRect();
    if (sceneBound.isEmpty())
        sceneBound = QRectF(-500, -500, 1000, 1000);

    qreal legendX = sceneBound.right() - 600;
    qreal legendY = sceneBound.bottom() - 100;

    QGraphicsTextItem *title = new QGraphicsTextItem("Status Demo");
    title->setPos(legendX - title->boundingRect().width() / 2, legendY);
    title->setDefaultTextColor(Qt::white);
    title->setFont(QFont("Arial", 14, QFont::Bold));
    addItem(title);

    QVector<QPair<QString, QColor>> legendItems = {
        {"Running", QColor(50, 200, 50)},
        {"Waiting", QColor(255, 165, 0)},
        {"InRunning", QColor(65, 105, 225)},
        {"NotRuing", QColor(120, 120, 120)},
        {"Skip", QColor(148, 0, 211)},
        {"failed", QColor(220, 20, 60)}};

    qreal totalWidth = 0;
    for (const auto &item : legendItems)
    {
        QGraphicsTextItem tempText(item.first);
        tempText.setFont(QFont("Arial", 12));
        totalWidth += 50 + tempText.boundingRect().width();
    }

    qreal startX = legendX - totalWidth / 2;

    for (int i = 0; i < legendItems.size(); i++)
    {
        qreal xPos = startX;
        for (int j = 0; j < i; j++)
        {
            QGraphicsTextItem tempText(legendItems[j].first);
            tempText.setFont(QFont("Arial", 12));
            xPos += 50 + tempText.boundingRect().width();
        }

        qreal yPos = legendY + 50;

        QGraphicsRectItem *colorRect = new QGraphicsRectItem(xPos, yPos, 20, 20);
        colorRect->setBrush(QBrush(legendItems[i].second));
        colorRect->setPen(Qt::NoPen);
        addItem(colorRect);

        QGraphicsTextItem *textItem = new QGraphicsTextItem(legendItems[i].first);
        textItem->setPos(xPos + 30, yPos);
        textItem->setDefaultTextColor(Qt::white);
        textItem->setFont(QFont("Arial", 12));
        addItem(textItem);
    }
}

void FlowChartScene::calculateNodeLevels()
{
    m_nodeLevels.clear();
    m_levelNodes.clear();
    m_maxLevel = 0;

    if (m_nodes.contains("begin"))
    {
        m_nodeLevels["begin"] = 0;
        m_levelNodes[0].append("begin");
    }

    std::queue<QString> nodeQueue;
    QSet<QString> visited;

    if (m_nodes.contains("begin"))
    {
        nodeQueue.push("begin");
        visited.insert("begin");
    }

    while (!nodeQueue.empty())
    {
        QString current = nodeQueue.front();
        nodeQueue.pop();

        for (FlowEdge *edge : m_edges)
        {
            if (edge->sourceNode()->id() == current)
            {
                QString neighbor = edge->targetNode()->id();
                if (!visited.contains(neighbor))
                {
                    visited.insert(neighbor);
                    m_nodeLevels[neighbor] = m_nodeLevels[current] + 1;
                    m_levelNodes[m_nodeLevels[neighbor]].append(neighbor);
                    m_maxLevel = qMax(m_maxLevel, m_nodeLevels[neighbor]);
                    nodeQueue.push(neighbor);
                }
            }
        }
    }

    bool changed;
    do
    {
        changed = false;
        QList<QPair<QString, int>> adjustments; // 存储需要调整的节点<节点ID, 新层级>

        for (FlowEdge *edge : m_edges)
        {
            QString source = edge->sourceNode()->id();
            QString target = edge->targetNode()->id();

            if (!m_nodeLevels.contains(source) || !m_nodeLevels.contains(target) || target == "begin")
                continue;

            if (m_nodeLevels[source] >= m_nodeLevels[target])
            {
                int newLevel = m_nodeLevels[source] + 1;
                adjustments.append(qMakePair(target, newLevel));
            }
        }

        // 应用调整并确保节点只在一个层级
        for (const auto &adj : adjustments)
        {
            QString nodeId = adj.first;
            int newLevel = adj.second;
            int oldLevel = m_nodeLevels.value(nodeId, -1);

            // 如果新层级大于旧层级才调整
            if (newLevel > oldLevel)
            {
                // 从旧层级移除
                if (oldLevel != -1 && m_levelNodes.contains(oldLevel))
                {
                    m_levelNodes[oldLevel].removeAll(nodeId);
                }

                // 添加到新层级
                m_nodeLevels[nodeId] = newLevel;
                m_levelNodes[newLevel].append(nodeId);
                m_maxLevel = qMax(m_maxLevel, newLevel);
                changed = true;
            }
        }
    } while (changed); // 循环直到没有变化

    // 处理孤立节点（未分配层级的节点）
    for (FlowNode *node : m_nodes)
    {
        if (!m_nodeLevels.contains(node->id()))
        {
            int newLevel = m_maxLevel + 1;
            m_nodeLevels[node->id()] = newLevel;
            m_levelNodes[newLevel].append(node->id());
            m_maxLevel = newLevel;
            qDebug() << "Isolated node assigned:" << node->id() << "at level" << newLevel;
        }
    }
}

void FlowChartScene::applySmartLayout()
{
    QMap<QString, QPointF> positions;

    int maxNodesInLevel = 0;
    for (int level = 0; level <= m_maxLevel; level++)
        maxNodesInLevel = qMax(maxNodesInLevel, m_levelNodes[level].size());

    double verticalSpacing = qMax(200.0, 700.0 / maxNodesInLevel);

    for (int level = 0; level <= m_maxLevel; level++)
    {
        const QList<QString> &nodes = m_levelNodes[level];
        int nodeCount = nodes.size();

        double startY = -((nodeCount - 1) * verticalSpacing) / 2.0;

        for (int i = 0; i < nodeCount; i++)
        {
            QString nodeId = nodes[i];
            double x = level * m_layerSpacing;
            double y = startY + i * verticalSpacing;
            positions[nodeId] = QPointF(x, y);
        }
    }

    applyForceDirectedLayout(50);

    for (auto it = positions.constBegin(); it != positions.constEnd(); ++it)
    {
        if (m_nodes.contains(it.key()))
            m_nodes[it.key()]->setPosition(it.value());
    }

    for (int level = 0; level <= m_maxLevel; level++)
    {
        const QList<QString> &nodes = m_levelNodes[level];
        for (const QString &nodeId : nodes)
        {
            FlowNode *node = m_nodes[nodeId];

            for (FlowEdge *edge : m_edges)
            {
                if (edge->targetNode()->id() == nodeId)
                {
                    FlowNode *sourceNode = edge->sourceNode();
                    if (sourceNode && node->x() < sourceNode->x())
                        positions[nodeId].setX(sourceNode->x() + m_layerSpacing);
                }
            }

            for (FlowEdge *edge : m_edges)
            {
                if (edge->sourceNode()->id() == nodeId)
                {
                    FlowNode *targetNode = edge->targetNode();
                    if (targetNode && node->x() > targetNode->x() && targetNode->id() != "begin")
                        positions[nodeId].setX(targetNode->x() - m_layerSpacing);
                }
            }
        }
    }

    QRectF sceneRect;
    for (FlowNode *node : m_nodes)
        sceneRect = sceneRect.united(node->sceneBoundingRect());
    setSceneRect(sceneRect.adjusted(-200, -200, 200, 200));
}

void FlowChartScene::applyForceDirectedLayout(int iterations)
{
    const double kRepulsion = 300.0;
    const double kAttraction = 0.2;
    const double kDamping = 0.8;
    const double minDistance = 50.0;

    QMap<QString, QPointF> positions;
    QMap<QString, QPointF> velocities;

    for (FlowNode *node : m_nodes)
    {
        positions[node->id()] = node->pos();
        velocities[node->id()] = QPointF(0, 0);
    }

    for (int iter = 0; iter < iterations; iter++)
    {
        QMap<QString, QPointF> forces;

        for (auto it1 = positions.constBegin(); it1 != positions.constEnd(); ++it1)
        {
            for (auto it2 = positions.constBegin(); it2 != positions.constEnd(); ++it2)
            {
                if (it1.key() == it2.key())
                    continue;

                QPointF delta = it2.value() - it1.value();
                double distance = std::max(1.0, std::sqrt(delta.x() * delta.x() + delta.y() * delta.y()));

                if (distance < minDistance)
                {
                    double force = kRepulsion / (distance * distance);
                    forces[it1.key()] -= delta * force;
                }
            }
        }

        for (FlowEdge *edge : m_edges)
        {
            QString sourceId = edge->sourceNode()->id();
            QString targetId = edge->targetNode()->id();

            QPointF delta = positions[targetId] - positions[sourceId];
            double distance = std::max(1.0, std::sqrt(delta.x() * delta.x() + delta.y() * delta.y()));

            double force = kAttraction * distance;
            forces[sourceId] += delta * force;
            forces[targetId] -= delta * force;
        }

        for (auto it = positions.begin(); it != positions.end(); ++it)
        {
            QString nodeId = it.key();
            QPointF force = forces.value(nodeId, QPointF(0, 0));

            velocities[nodeId] = (velocities[nodeId] + force) * kDamping;
            positions[nodeId] += velocities[nodeId];
        }
    }

    for (auto it = positions.constBegin(); it != positions.constEnd(); ++it)
    {
        if (m_nodes.contains(it.key()))
            m_nodes[it.key()]->setPosition(it.value());
    }
}

void FlowChartScene::updateEdgePaths()
{
    // 按源节点分组（出边）- 原有逻辑保留
    QMap<FlowNode *, QList<FlowEdge *>> sourceEdgeGroups;
    // 按目标节点分组（入边）- 用于计算目标节点偏移
    QMap<FlowNode *, QList<FlowEdge *>> targetEdgeGroups;

    for (FlowEdge *edge : m_edges)
    {
        sourceEdgeGroups[edge->sourceNode()].append(edge);
        targetEdgeGroups[edge->targetNode()].append(edge); // 新增目标节点分组
    }

    // 为每条边分配源节点通道索引和目标节点通道索引
    for (auto &sourceGroup : sourceEdgeGroups)
    {
        // 按目标节点位置排序（Y坐标）
        std::sort(sourceGroup.begin(), sourceGroup.end(), [](FlowEdge *a, FlowEdge *b)
                  { return a->targetNode()->pos().y() < b->targetNode()->pos().y(); });

        for (int i = 0; i < sourceGroup.size(); ++i)
        {
            FlowEdge *edge = sourceGroup[i];
            FlowNode *targetNode = edge->targetNode();

            // 计算源节点通道索引（原有）
            int sourceIndex = i;
            int sourceTotal = sourceGroup.size();

            // 计算目标节点通道索引
            QList<FlowEdge *> targetGroup = targetEdgeGroups[targetNode];
            // 对目标节点的入边按源节点ID排序
            std::sort(targetGroup.begin(), targetGroup.end(), [](FlowEdge *a, FlowEdge *b)
                      { return a->sourceNode()->id() < b->sourceNode()->id(); });
            int targetIndex = targetGroup.indexOf(edge);
            int targetTotal = targetGroup.size();

            // 同时设置源和目标通道信息
            edge->setChannelIndices(sourceIndex, sourceTotal, targetIndex, targetTotal);
        }
    }

    // 更新所有边路径
    for (FlowEdge *edge : m_edges)
    {
        edge->updatePath();
    }
}

NodeState FlowChartScene::mapJsonStatusToNodeState(const QString &status)
{
    if (status == "DISABLE")
        return DisabledState;
    if (status == "WORKING")
        return ActiveState;
    if (status == "ABORT")
        return ErrorState;
    return NormalState;
}

//! [6]
void FlowChartScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (mouseEvent->button() != Qt::LeftButton)
        return;

//     DiagramItem *item;
//     switch (myMode) {
//         case InsertItem:
//             item = new DiagramItem(myItemType, myItemMenu);
//             item->setBrush(myItemColor);
//             addItem(item);
//             item->setPos(mouseEvent->scenePos());
//             emit itemInserted(item);
//             break;
// //! [6] //! [7]
//         case InsertLine:
//             line = new QGraphicsLineItem(QLineF(mouseEvent->scenePos(),
//                                         mouseEvent->scenePos()));
//             line->setPen(QPen(myLineColor, 2));
//             addItem(line);
//             break;
// //! [7] //! [8]
//         case InsertText:
//             textItem = new DiagramTextItem();
//             textItem->setFont(myFont);
//             textItem->setTextInteractionFlags(Qt::TextEditorInteraction);
//             textItem->setZValue(1000.0);
//             connect(textItem, SIGNAL(lostFocus(DiagramTextItem*)),
//                     this, SLOT(editorLostFocus(DiagramTextItem*)));
//             connect(textItem, SIGNAL(selectedChange(QGraphicsItem*)),
//                     this, SIGNAL(itemSelected(QGraphicsItem*)));
//             addItem(textItem);
//             textItem->setDefaultTextColor(myTextColor);
//             textItem->setPos(mouseEvent->scenePos());
//             emit textInserted(textItem);
// //! [8] //! [9]
//     default:
//         ;
//     }
    QGraphicsScene::mousePressEvent(mouseEvent);
}
//! [9]

//! [10]
void FlowChartScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    // if (myMode == InsertLine && line != 0) {
    //     QLineF newLine(line->line().p1(), mouseEvent->scenePos());
    //     line->setLine(newLine);
    // } else if (myMode == MoveItem) {
        
    // }

    QGraphicsScene::mouseMoveEvent(mouseEvent);
}
//! [10]

//! [11]
void FlowChartScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
//     if (line != 0 && myMode == InsertLine) {
//         QList<QGraphicsItem *> startItems = items(line->line().p1());
//         if (startItems.count() && startItems.first() == line)
//             startItems.removeFirst();
//         QList<QGraphicsItem *> endItems = items(line->line().p2());
//         if (endItems.count() && endItems.first() == line)
//             endItems.removeFirst();

//         removeItem(line);
//         delete line;
// //! [11] //! [12]

//         if (startItems.count() > 0 && endItems.count() > 0 &&
//             startItems.first()->type() == DiagramItem::Type &&
//             endItems.first()->type() == DiagramItem::Type &&
//             startItems.first() != endItems.first()) {
//             DiagramItem *startItem = qgraphicsitem_cast<DiagramItem *>(startItems.first());
//             DiagramItem *endItem = qgraphicsitem_cast<DiagramItem *>(endItems.first());
//             Arrow *arrow = new Arrow(startItem, endItem);
//             arrow->setColor(myLineColor);
//             startItem->addArrow(arrow);
//             endItem->addArrow(arrow);
//             arrow->setZValue(-1000.0);
//             addItem(arrow);
//             arrow->updatePosition();
//         }
//     }
//! [12] //! [13]
    //line = 0;
    QGraphicsScene::mouseReleaseEvent(mouseEvent);
}