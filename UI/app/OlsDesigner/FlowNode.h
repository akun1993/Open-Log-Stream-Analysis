#ifndef FLOWNODE_H
#define FLOWNODE_H

#include <QGraphicsRectItem>
#include <QBrush>
#include <QPen>
#include <QFont>
#include "PluginInfo.h"

class FlowEdge;

class FlowNode : public QGraphicsRectItem
{
public:

    enum { Type = UserType + 15 };


    explicit FlowNode(PluginType type, int id, const QString &label, QMenu *contextMenu,QGraphicsItem *parent = nullptr);

    int id() const { return m_id; }
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
    PluginType pluginType() const { return m_pluginType; }

    void addArrow(FlowEdge *arrow);

    int type() const override { return Type; }

    QPixmap image() const;

    void openPropertyView();

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

    // void mousePressEvent(QGraphicsSceneMouseEvent *event);

    // void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);


private:
    int m_id;
    QString m_label;

    PluginType m_pluginType;
    QMenu *myContextMenu;


    QVector<FlowEdge *> arrows;

    //NodeState m_state = DisabledState;

    QList<FlowEdge *> m_inEdges;
    QList<FlowEdge *> m_outEdges;


    int m_state = 0;


    bool m_hasActivated = false;
    bool m_isSkip = false;
    int m_level = 0;


    QBrush m_inactiveBrush;
    QBrush m_activeBrush;

    QBrush m_inputBrush;

    QBrush m_processBrush;

    QBrush m_outputBrush;

    QPen m_inputPen;
    QPen m_processPen;
    QPen m_outputPen;


    QPen m_inactivePen;

    QPen m_activePen;

    QFont m_font;


    static constexpr double NODE_WIDTH = 190;
    static constexpr double NODE_HEIGHT = 130;
};

#endif // FLOWNODE_H
