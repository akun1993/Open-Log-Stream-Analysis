#ifndef OLSAPP_H
#define OLSAPP_H
#include <QApplication>
#include <QPointer>
#include "OlsMainwindow.h"

class OlsApp : public QApplication {
public:
    OlsApp(int& argc, char** argv);
    ~OlsApp();
    inline QMainWindow* GetMainWindow() const { return m_mainWindow.data(); }

private:
    QPointer<OlsMainWindow> m_mainWindow;
};

inline OlsApp* App()
{
    return static_cast<OlsApp*>(qApp);
}

#endif // OLSAPP_H
