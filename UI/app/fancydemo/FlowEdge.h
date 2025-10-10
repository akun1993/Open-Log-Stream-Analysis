#ifndef FLOWEDGE_H
#define FLOWEDGE_H

#include <QGraphicsPathItem>
#include <QPen>
#include <QGraphicsEllipseItem>


class FlowNode;

class FlowEdge : public QGraphicsPathItem
{
public:
    FlowEdge(FlowNode *source, FlowNode *target, const QString &label,
             QGraphicsItem *parent = nullptr);

    void updatePath();
   // void setState(NodeState state);
   // NodeState state() const { return m_state; }

    FlowNode *sourceNode() const { return m_source; }
    FlowNode *targetNode() const { return m_target; }
    QString label() const { return m_label; }

    // 新增：同时设置源和目标通道索引
    void setChannelIndices(int sourceIndex, int sourceTotal, int targetIndex, int targetTotal)
    {
        m_sourceChannelIndex = sourceIndex;
        m_sourceTotalChannels = sourceTotal;
        m_targetChannelIndex = targetIndex;
        m_targetTotalChannels = targetTotal;
    }

    void setHasActivated(bool activated) { m_hasActivated = activated; }
    bool hasActivated() const { return m_hasActivated; }

private:
    /// @brief 绘制正交路径
    void drawOrthogonalPath();
    /// @brief 更新源点位置
    void updateSourceDotPosition();
    /// @brief 检查路径是否与节点冲突
    /// @param path 路径
    /// @param allNodes 所有节点
    /// @return 是否冲突
    bool pathCollidesWithNodes(const QPainterPath &path, const QList<FlowNode *> &allNodes);
    /// @brief  计算路径与节点的碰撞区域
    /// @param region 区域
    /// @param allNodes 所有节点
    /// @return 区域密度
    double calculateRegionDensity(const QRectF &region, const QList<FlowNode *> &allNodes);
    /// @brief 计算最佳绕行方向(长+中距离)
    /// @param start 起始点
    /// @param end 结束点
    /// @param sceneCenter 场景中心
    /// @param allNodes 所有节点
    /// @return 最佳垂直偏移量
    double calculateBestVerticalOffset(const QPointF &start, const QPointF &end, const QPointF &sceneCenter, const QList<FlowNode *> &allNodes);
    /// @brief 计算绕行方向 (中距离)
    /// @param start 起始点
    /// @param end 结束点
    /// @param baseOffset 基础偏移量
    /// @return 方向偏移量
    double calculateDirectionalOffset(const QPointF &start, const QPointF &end, double baseOffset);

private:
    FlowNode *m_source;  // 源节点
    FlowNode *m_target;  // 目标节点
    QString m_label;     // 边的标签文本
    //NodeState m_state;   // 边的状态
    bool m_hasActivated; // 激活状态标志

    QGraphicsEllipseItem *m_sourceDot; // 出边原点标志
    const qreal DOT_RADIUS = 5.0;      // 原点半径

    // 画笔样式
    QPen m_activatedPen;
    QPen m_activePen;
    QPen m_disabledPen;
    QPen m_errorPen;

    int m_sourceChannelIndex;  // 源节点出边索引
    int m_sourceTotalChannels; // 源节点总出边数
    int m_targetChannelIndex;  // 目标节点入边索引
    int m_targetTotalChannels; // 目标节点总入边数
};

#endif // FLOWEDGE_H


