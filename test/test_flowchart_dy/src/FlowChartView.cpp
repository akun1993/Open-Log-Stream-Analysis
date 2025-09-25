#include "FlowChartView.h"
#include "FlowChartScene.h"
#include <QWheelEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTimer>
#include <QTransform>

FlowChartView::FlowChartView(QWidget *parent)
    : QGraphicsView(parent),
      m_minScale(0.1), // 默认最小缩放为10%
      m_maxScale(5.0)  // 默认最大缩放为500%
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::TextAntialiasing);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    m_scene = new FlowChartScene(this);
    setScene(m_scene);
}

double FlowChartView::currentScale() const
{
    // 从变换矩阵中获取当前缩放比例
    return transform().m11();
}

void FlowChartView::loadFlowChart(const QJsonObject &json)
{
    m_scene->loadFlowChart(json);
}

void FlowChartView::wheelEvent(QWheelEvent *event)
{
    double current = currentScale();
    const double scaleFactor = 1.15;
    double newScale = event->angleDelta().y() > 0 ? current * scaleFactor : current / scaleFactor;

    if (newScale >= m_minScale && newScale <= m_maxScale)
    {
        scale(event->angleDelta().y() > 0 ? scaleFactor : 1.0 / scaleFactor,
              event->angleDelta().y() > 0 ? scaleFactor : 1.0 / scaleFactor);
        m_isUserZoomed = true; // 标记用户缩放
    }
    event->accept();
}

void FlowChartView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (scene() && !m_isUserZoomed)
    {
        fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    }
}

//! [3]
void FlowChartView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    // case Qt::Key_Up:
    //     centerNode->moveBy(0, -20);
    //     break;
    // case Qt::Key_Down:
    //     centerNode->moveBy(0, 20);
    //     break;
    // case Qt::Key_Left:
    //     centerNode->moveBy(-20, 0);
    //     break;
    // case Qt::Key_Right:
    //     centerNode->moveBy(20, 0);
    //     break;
    // case Qt::Key_Plus:
    //     zoomIn();
    //     break;
    // case Qt::Key_Minus:
    //     zoomOut();
    //     break;
    // case Qt::Key_Space:
    // case Qt::Key_Enter:
    //     shuffle();
    //     break;
    default:
        QGraphicsView::keyPressEvent(event);
    }
}
