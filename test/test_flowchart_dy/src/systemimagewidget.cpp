#include "systemimagewidget.h"
#include "ui_systemimagewidget.h"
#include <QJsonArray>
#include <QTimer>
#include <QRandomGenerator>
#include <QDebug>
SystemImageWidget::SystemImageWidget(QWidget *parent) : QWidget(parent),
                                                        ui(new Ui::SystemImageWidget)
{
    ui->setupUi(this);
    flowChartView = new FlowChartView(this);
    ui->verticalLayout->addWidget(flowChartView);
}

SystemImageWidget::~SystemImageWidget()
{
    delete ui;
}

void SystemImageWidget::loadFlowData(QJsonObject &flow)
{
    flowChartView->loadFlowChart(flow);
}
