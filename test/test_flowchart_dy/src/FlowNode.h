#ifndef FLOWNODE_H
#define FLOWNODE_H

#include <QGraphicsRectItem>
#include <QBrush>
#include <QPen>
#include <QFont>
#include "FlowChartScene.h"

class FlowNode : public QGraphicsRectItem
{
public:
    explicit FlowNode(const QString &id, const QString &label, QGraphicsItem *parent = nullptr);

    QString id() const { return m_id; }
    QString label() const { return m_label; }

    void setState(NodeState state);

    void setPosition(const QPointF &pos);

    void addOutEdge(FlowEdge *edge) { m_outEdges.append(edge); }

    void addInEdge(FlowEdge *edge) { m_inEdges.append(edge); }

    const QList<FlowEdge *> &outEdges() const { return m_outEdges; }

    const QList<FlowEdge *> &inEdges() const { return m_inEdges; }

    NodeState state() const { return m_state; }


    void setHasActivated(bool activated) { m_hasActivated = activated; }
    bool hasActivated() const { return m_hasActivated; }

    void setSkip(bool skip) { m_isSkip = skip; }
    bool isSkip() const { return m_isSkip; }

    int level() const { return m_level; }
    void setLevel(int level) { m_level = level; }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

    // void mousePressEvent(QGraphicsSceneMouseEvent *event);

    // void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    //! [12]



private:
    QString m_id;                    // 节点唯一标识
    QString m_label;                 // 节点显示文本
    NodeState m_state = DisabledState; // 节点当前状态

    QList<FlowEdge *> m_inEdges;    // in edage 列表
    QList<FlowEdge *> m_outEdges;    // 出边列表

    bool m_hasActivated = false;     // 节点是否已激活
    bool m_isSkip = false;           // 节点是否被跳过
    int m_level = 0;

    // 节点不同状态下的样式
    QBrush m_normalBrush;
    QBrush m_activeBrush;

    QBrush m_disabledBrush;
    QBrush m_errorBrush;
    QBrush m_activatedBrush;
    QBrush m_skipBrush;

    QPen m_skipPen;
    QPen m_activePen;
    QPen m_waitingPen;
    QPen m_disabledPen;
    QPen m_errorPen;
    QPen m_activatedPen;

    QFont m_font; // 节点文本字体

    static constexpr double NODE_WIDTH = 200; // 节点默认宽度
    static constexpr double NODE_HEIGHT = 150; // 节点默认高度
};

#endif // FLOWNODE_H
