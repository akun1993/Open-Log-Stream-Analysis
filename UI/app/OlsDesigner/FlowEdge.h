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

    //
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
    /// @brief
    void drawOrthogonalPath();
    /// @brief
    void updateSourceDotPosition();
    /// @brief
    /// @param path
    /// @param allNodes
    /// @return
    bool pathCollidesWithNodes(const QPainterPath &path, const QList<FlowNode *> &allNodes);
    /// @brief
    /// @param region
    /// @param allNodes
    /// @return
    double calculateRegionDensity(const QRectF &region, const QList<FlowNode *> &allNodes);
    /// @brief
    /// @param start
    /// @param end
    /// @param sceneCenter
    /// @param allNodes
    /// @return
    double calculateBestVerticalOffset(const QPointF &start, const QPointF &end, const QPointF &sceneCenter, const QList<FlowNode *> &allNodes);
    /// @brief
    /// @param start
    /// @param end
    /// @param baseOffset
    /// @return
    double calculateDirectionalOffset(const QPointF &start, const QPointF &end, double baseOffset);

private:
    FlowNode *m_source;
    FlowNode *m_target;
    QString m_label;
    //NodeState m_state;
    bool m_hasActivated;

    QGraphicsEllipseItem *m_sourceDot;
    const qreal DOT_RADIUS = 5.0;


    QPen m_activatedPen;
    QPen m_activePen;
    QPen m_disabledPen;
    QPen m_errorPen;

    int m_sourceChannelIndex;
    int m_sourceTotalChannels;
    int m_targetChannelIndex;
    int m_targetTotalChannels;
};

#endif // FLOWEDGE_H


