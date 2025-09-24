#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
// #include "clusterclient.h"

// class HmiClient;
class SystemImageWidget;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  void initUI();
  void initGrpcClient();

signals:
  void safeUpdateWorkingStatus(const QString &status);
private slots:
  // void MonitorCallback(const task_flow::GraphInfo &graph_info);
  void handleeWorkingStatus(const QString &jsonData);

private:
  // QSharedPointer<HmiClient> m_clusterclient;
  Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
