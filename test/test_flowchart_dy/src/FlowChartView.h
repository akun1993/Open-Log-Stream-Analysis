#ifndef FLOWCHARTVIEW_H
#define FLOWCHARTVIEW_H

#include <QGraphicsView>
#include <QJsonObject>

class FlowChartScene;

class FlowChartView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit FlowChartView(QWidget *parent = nullptr);

    void loadFlowChart(const QJsonObject &json);

protected:

    void keyPressEvent(QKeyEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    FlowChartScene *m_scene;
    double m_minScale; // 最小缩放比例
    double m_maxScale; // 最大缩放比例
    bool m_isUserZoomed = false;
    // 获取当前缩放比例
    double currentScale() const;
};

#endif // FLOWCHARTVIEW_H
