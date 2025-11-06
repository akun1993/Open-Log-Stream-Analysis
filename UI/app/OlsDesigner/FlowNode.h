#ifndef FLOWNODE_H
#define FLOWNODE_H

#include <QGraphicsRectItem>
#include <QBrush>
#include <QPen>
#include <QFont>
//#include "FlowChartScene.h"

class FlowEdge;

class FlowNode : public QGraphicsRectItem
{
public:

    enum { Type = UserType + 15 };

    enum FlowNodeType { INPUT, PROCESS, OUTPUT};

    explicit FlowNode(FlowNodeType nodeType, const QString &label, QMenu *contextMenu,QGraphicsItem *parent = nullptr);

   // QString id() const { return m_id; }
    QString label() const { return m_label; }

   // void setState(NodeState state);

    void setPosition(const QPointF &pos);

    void addOutEdge(FlowEdge *edge) { m_outEdges.append(edge); }

    void addInEdge(FlowEdge *edge) { m_inEdges.append(edge); }

    const QList<FlowEdge *> &outEdges() const { return m_outEdges; }

    const QList<FlowEdge *> &inEdges() const { return m_inEdges; }

   // NodeState state() const { return m_state; }


    void setHasActivated(bool activated) { m_hasActivated = activated; }
    bool hasActivated() const { return m_hasActivated; }

    void setSkip(bool skip) { m_isSkip = skip; }
    bool isSkip() const { return m_isSkip; }

    int level() const { return m_level; }
    void setLevel(int level) { m_level = level; }

    void removeArrow(FlowEdge *arrow);
    void removeArrows();
    FlowNodeType flowNodeType() const { return myFlowNodeType; }

    void addArrow(FlowEdge *arrow);

    int type() const override { return Type; }

    QPixmap image() const;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

    // void mousePressEvent(QGraphicsSceneMouseEvent *event);

    // void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);


private:
    QString m_id;
    QString m_label;

    FlowNodeType myFlowNodeType;
    QMenu *myContextMenu;


    QVector<FlowEdge *> arrows;

    //NodeState m_state = DisabledState;

    QList<FlowEdge *> m_inEdges;
    QList<FlowEdge *> m_outEdges;


    int m_state = 0;


    bool m_hasActivated = false;
    bool m_isSkip = false;
    int m_level = 0;


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

    QFont m_font;

    static constexpr double NODE_WIDTH = 160;
    static constexpr double NODE_HEIGHT = 100;
};

#endif // FLOWNODE_H
