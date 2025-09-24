#include <QApplication>
#include <QIcon>
#include <QSharedMemory>
#include <QMessageBox>
#include <QProcess>
#include "mainwindow.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QString uniqueId = "flowchartAppId";
    QSharedMemory sharedMemory(uniqueId);
    QApplication::setWindowIcon(QIcon(":/logo"));
    // 尝试附加到共享内存
    if (sharedMemory.attach())
    {
        qint64 *runningInstance = static_cast<qint64 *>(sharedMemory.data());
        if (runningInstance && *runningInstance != 0)
        {
            // 检查进程ID是否有效
            QProcess process;
            process.start("pidof", QStringList() << QString::number(*runningInstance));
            process.waitForFinished();
            if (!process.readAllStandardOutput().isEmpty())
            {
                // 进程ID有效，说明有实例在运行
                QMessageBox::warning(nullptr, QObject::tr("警告"),
                                     QObject::tr("程序已经在运行！"));
                sharedMemory.detach();
                return 0;
            }
        }
        // 进程ID无效或共享内存内容为0，说明没有实例在运行
        sharedMemory.detach();
    }

    // 尝试创建共享内存段，大小为sizeof(qint64)
    if (sharedMemory.create(sizeof(qint64)))
    {
        qint64 *runningInstance = static_cast<qint64 *>(sharedMemory.data());
        *runningInstance = QCoreApplication::applicationPid();
        // **** 窗口启动代码*****//
        MainWindow w;
        w.show();
        //******* end ********//
        //  程序正常退出时，设置共享内存内容为0
        QObject::connect(&a, &QApplication::aboutToQuit, [&]()
                         {
            if (sharedMemory.isAttached())
            {
                qint64 *runningInstance = static_cast<qint64*>(sharedMemory.data());
                if (runningInstance) *runningInstance = 0;
                sharedMemory.detach();
            } });

        return a.exec();
    }
    else
    {
        QMessageBox::information(nullptr, QObject::tr("提示"),
                                 QObject::tr("程序已经在运行！"));
        return 1;
    }
}