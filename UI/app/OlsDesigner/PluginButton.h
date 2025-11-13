#ifndef PLUGINBUTTON_H
#define PLUGINBUTTON_H

#include <QToolButton>
#include <QString>
#include "PluginInfo.h"

class PluginButton : public QToolButton
{
    Q_OBJECT
public:
    PluginButton(PluginType type, QWidget *parent = nullptr);

    void setName(QString name){
        m_name = name;
    }

    QString name() const {
        return m_name;
    }

    void setId(int id){
        m_id = id;
    }

    int id(){
        return m_id;
    }

private:
    PluginType m_type{PLUGIN_INVALID};
    int m_id{-1};
    QString m_name;
};

#endif // PLUGINBUTTON_H
