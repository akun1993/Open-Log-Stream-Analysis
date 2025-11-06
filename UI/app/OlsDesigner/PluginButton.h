#ifndef PLUGINBUTTON_H
#define PLUGINBUTTON_H

#include <QToolButton>
#include <QString>

class PluginButton : public QToolButton
{
    Q_OBJECT
public:
    PluginButton(QWidget *parent = nullptr);

    void setName(QString name){
        m_name = name;
    }

    QString name() const {
        return m_name;
    }

private:

    QString m_name;
};

#endif // PLUGINBUTTON_H
