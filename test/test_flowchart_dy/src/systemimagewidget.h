#ifndef SYSTEMIMAGEWIDGET_H
#define SYSTEMIMAGEWIDGET_H

#include <QWidget>
#include "FlowChartView.h"
namespace Ui
{
    class SystemImageWidget;
}

class SystemImageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SystemImageWidget(QWidget *parent = nullptr);
    ~SystemImageWidget();
    void loadFlowData(QJsonObject &flow);

private:
    Ui::SystemImageWidget *ui;
    FlowChartView *flowChartView;
};

#endif // SYSTEMIMAGEWIDGET_H
