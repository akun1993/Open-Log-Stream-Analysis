#include "olsapp.h"

OLSApp::OLSApp(int& argc, char** argv)
    : QApplication(argc, argv)
{

    m_mainWindow = new OLSMainWindow;
    m_mainWindow->setGeometry(100, 100, 800, 500);
    m_mainWindow->show();
}

OLSApp::~OLSApp()
{
}
