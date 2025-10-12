#ifndef FLOWCHARTVIEW_H
#define FLOWCHARTVIEW_H

#include <QGraphicsView>
#include <QJsonObject>

class QMenu;
class FlowChartScene;

class FlowChartView : public QGraphicsView
{
    Q_OBJECT
public:
    FlowChartView( QGraphicsScene *secne,QWidget *parent = nullptr);

    void loadFlowChart(const QJsonObject &json);

protected:

    void keyPressEvent(QKeyEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    //FlowChartScene *m_scene;
    double m_minScale;
    double m_maxScale;
    bool m_isUserZoomed = false;

    double currentScale() const;
};

#endif // FLOWCHARTVIEW_H
