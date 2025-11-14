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


inline config_t *GetGlobalConfig()
{
	return App()->GlobalConfig();
}

std::vector<std::pair<std::string, std::string>> GetLocaleNames();
inline const char *Str(const char *lookup)
{
	return App()->GetString(lookup);
}

#define QTStr(lookupVal) QString::fromUtf8(Str(lookupVal))

#endif // OLSAPP_H
