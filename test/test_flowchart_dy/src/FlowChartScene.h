#ifndef FLOWCHARTSCENE_H
#define FLOWCHARTSCENE_H

#include <QGraphicsScene>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QPointF>
#include <QList>

enum NodeState
{
    NormalState,   // 默认状态（空闲）
    ActiveState,   // 激活状态（运行中）
    DisabledState, // 禁用状态
    ErrorState     // 错误状态
};

class FlowNode;
class FlowEdge;

class FlowChartScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit FlowChartScene(QObject *parent = nullptr);

    void loadFlowChart(const QJsonObject &json);

    void arrangeLayout();
    NodeState mapJsonStatusToNodeState(const QString &status);


protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) override;

private:
    void parseJson(const QJsonObject &json);
    void clearScene();
    void createNodes();
    void createEdges();
    void calculateNodeLevels();
    void applySmartLayout();
    void applyForceDirectedLayout(int iterations = 100);
    void updateEdgePaths();

    void createLegend();

private:
    QJsonObject m_jsonData;                 // 存储流程图的JSON数据
    QMap<QString, FlowNode *> m_nodes;      // 节点ID到节点对象的映射
    QList<FlowEdge *> m_edges;              // 所有边的列表
    QMap<QString, NodeState> m_nodeStates;  // 节点ID到状态的映射
    QMap<QString, int> m_nodeLevels;        // 节点ID到层级的映射
    QMap<int, QList<QString>> m_levelNodes; // 层级到节点ID列表的映射
    QMap<QString, bool> m_nodeSkips;        // 节点ID到是否跳过的映射
    double m_layerSpacing = 250;            // 层级之间的水平间距
    int m_maxLevel = 0;                     // 最大层级数
};

#endif // FLOWCHARTSCENE_H