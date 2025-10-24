#include "OlsApp.h"

OlsApp::OlsApp(int& argc, char** argv)
    : QApplication(argc, argv)
{

    m_mainWindow = new OlsMainWindow;
    m_mainWindow->setGeometry(100, 100, 1500, 800);
    m_mainWindow->show();
}

OlsApp::~OlsApp()
{
    delete m_mainWindow;
}
