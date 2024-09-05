#ifndef OLSAPP_H
#define OLSAPP_H
#include <QApplication>
#include <QPointer>
#include <mainwindow.h>

class OLSApp : public QApplication {
public:
    OLSApp(int& argc, char** argv);
    ~OLSApp();
    inline QMainWindow* GetMainWindow() const { return m_mainWindow.data(); }

private:
    QPointer<OLSMainWindow> m_mainWindow;
};

inline OLSApp* App()
{
    return static_cast<OLSApp*>(qApp);
}

#endif // OLSAPP_H
