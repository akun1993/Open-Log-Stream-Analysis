#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "systemimagewidget.h"
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QScreen>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  initUI();
  initGrpcClient();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::initUI() {
  setWindowTitle("工序流程图");
  QScreen *screen = QGuiApplication::primaryScreen();
  QRect screenRect = screen->geometry();
  int x = (screenRect.width() - width()) / 2;
  int y = (screenRect.height() - height()) / 2;
  move(x, y);
  setMinimumSize(800, 600);
}

void MainWindow::initGrpcClient() {
  //     auto initClinet = [=](QString ip) -> QSharedPointer<HmiClient>
  //     {
  //         QString congfig = QString();
  // #ifdef USE_PACKAGE_ENABLE
  //         congfig = QCoreApplication::applicationDirPath() +
  //         "/config/config.ini";
  // #else
  //         congfig = CONFIG_FILE_PATH;
  // #endif
  //         QSettings settings(congfig, QSettings::IniFormat);
  //         auto portIP =
  //         settings.value(QString("client/%1").arg(ip)).toString().toStdString();
  //         return QSharedPointer<HmiClient>(new HmiClient(portIP));
  //     };
  //     m_clusterclient = initClinet("portIP");
  //     auto cb_graph_info = std::bind(&MainWindow::MonitorCallback, this,
  //                                    std::placeholders::_1);
  //     m_clusterclient->setCallback(cb_graph_info);
  //     if (m_clusterclient->start() == 1)
  //     {
  //         qDebug() << "客户端初始化成功！";
  //     }

  //     connect(this, &MainWindow::safeUpdateWorkingStatus,
  //             this, &MainWindow::handleeWorkingStatus, Qt::QueuedConnection);

  QFile file("/home/V01/uidq8743/OpenSource/sharecode-master/flowchart_dy/"
             "config/test.json");

  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {

    QByteArray jsonData = file.readAll();
    file.close(); // 关闭文件

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (jsonDoc.isNull()) {
      qDebug() << "Could not parse JSON document";
      return;
    } else {
      QString str = jsonData;
      handleeWorkingStatus(str);
    }

  } else {
    qDebug() << "无法打开文件:" << file.errorString();
  }
}

// void MainWindow::MonitorCallback(const task_flow::GraphInfo &graph_info) {
//   //   std::string json = protoMessageToJson(graph_info);
//   //   emit safeUpdateWorkingStatus(QString::fromStdString(json));
// }

void MainWindow::handleeWorkingStatus(const QString &jsonData) {

  QJsonObject jsonObject = QJsonDocument::fromJson(jsonData.toUtf8()).object();
  ui->widget->loadFlowData(jsonObject);
}