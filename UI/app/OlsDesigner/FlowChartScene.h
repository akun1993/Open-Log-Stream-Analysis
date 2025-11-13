

#ifndef DIAGRAMSCENE_H
#define DIAGRAMSCENE_H

#include "FlowNode.h"
#include <QGraphicsScene>

QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
class QMenu;
class QPointF;
class QGraphicsLineItem;
class QFont;
class QGraphicsTextItem;
class QColor;
QT_END_NAMESPACE

//! [0]
class FlowChartScene : public QGraphicsScene
{
    Q_OBJECT

public:
    enum Mode { InsertItem, InsertLine, MoveItem };

    explicit FlowChartScene(QMenu *itemMenu, QObject *parent = nullptr);
    QFont font() const { return myFont; }
    QColor itemColor() const { return myItemColor; }
    QColor lineColor() const { return myLineColor; }
    void setLineColor(const QColor &color);
    void setTextColor(const QColor &color);
    void setItemColor(const QColor &color);
   // void setFont(const QFont &font);

public Q_SLOTS:
    void setMode(Mode mode);

    void setItemInfo(PluginType type, int id,const QString &name);

Q_SIGNALS:
    void itemInserted(FlowNode *item);
    void itemSelected(QGraphicsItem *item);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) override;

private:
    bool isItemChange(int type) const;

    PluginType myType;
    int      myId;
    QString myItemName;

    QMenu *myItemMenu;
    Mode myMode;
    bool leftButtonDown;
    QPointF startPoint;
    QGraphicsLineItem *line;
    QFont myFont;
    QColor myItemColor;
    QColor myLineColor;
};
//! [0]

#endif // DIAGRAMSCENE_H
