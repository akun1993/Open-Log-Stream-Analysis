#pragma once

#include "vertical-scroll-area.h"
#include <QPointer>
#include <memory>
#include "ols-data.h"
#include "ols-properties.h"
#include "ols.hpp"
#include <qtimer.h>
#include <vector>

class QFormLayout;
class OLSPropertiesView;
class QLabel;

typedef ols_properties_t* (*PropertiesReloadCallback)(void* obj);
typedef void (*PropertiesUpdateCallback)(void* obj, ols_data_t* old_settings,
    ols_data_t* new_settings);
typedef void (*PropertiesVisualUpdateCb)(void* obj, ols_data_t* settings);

/* ------------------------------------------------------------------------- */

class WidgetInfo : public QObject {
    Q_OBJECT

    friend class OLSPropertiesView;

private:
    OLSPropertiesView* view;
    ols_property_t* property;
    QWidget* widget;
    QPointer<QTimer> update_timer;
    bool recently_updated = false;
    OLSData old_settings_cache;

    void BoolChanged(const char* setting);
    void IntChanged(const char* setting);
    void FloatChanged(const char* setting);
    void TextChanged(const char* setting);
    bool PathChanged(const char* setting);
    void ListChanged(const char* setting);
    bool ColorChangedInternal(const char* setting, bool supportAlpha);
    bool ColorChanged(const char* setting);
    bool ColorAlphaChanged(const char* setting);
    bool FontChanged(const char* setting);
    void GroupChanged(const char* setting);
    void EditableListChanged();
    void ButtonClicked();

    void TogglePasswordText(bool checked);

public:
    inline WidgetInfo(OLSPropertiesView* view_, ols_property_t* prop,
        QWidget* widget_)
        : view(view_)
        , property(prop)
        , widget(widget_)
    {
    }

    ~WidgetInfo()
    {
        if (update_timer) {
            update_timer->stop();
            QMetaObject::invokeMethod(update_timer, "timeout");
            update_timer->deleteLater();
        }
    }

public slots:

    void ControlChanged();

    /* editable list */
    void EditListAdd();
    void EditListAddText();
    void EditListAddFiles();
    void EditListAddDir();
    void EditListRemove();
    void EditListEdit();
    void EditListUp();
    void EditListDown();
    void EditListReordered(const QModelIndex& parent, int start, int end,
        const QModelIndex& destination, int row);
};

/* ------------------------------------------------------------------------- */

class OLSPropertiesView : public VScrollArea {
    Q_OBJECT

    friend class WidgetInfo;

    using properties_delete_t = decltype(&ols_properties_destroy);
    using properties_t = std::unique_ptr<ols_properties_t, properties_delete_t>;

private:
    QWidget* widget = nullptr;
    properties_t properties;
    OLSData settings;
    OLSWeakObjectAutoRelease weakObj;
    void* rawObj = nullptr;
    std::string type;
    PropertiesReloadCallback reloadCallback;
    PropertiesUpdateCallback callback = nullptr;
    PropertiesVisualUpdateCb visUpdateCb = nullptr;
    int minSize;
    std::vector<std::unique_ptr<WidgetInfo>> children;
    std::string lastFocused;
    QWidget* lastWidget = nullptr;
    bool deferUpdate;

    QWidget* NewWidget(ols_property_t* prop, QWidget* widget,
        const char* signal);

    QWidget* AddCheckbox(ols_property_t* prop);
    QWidget* AddText(ols_property_t* prop, QFormLayout* layout,
        QLabel*& label);
    void AddPath(ols_property_t* prop, QFormLayout* layout, QLabel** label);
    void AddInt(ols_property_t* prop, QFormLayout* layout, QLabel** label);
    void AddFloat(ols_property_t* prop, QFormLayout* layout,
        QLabel** label);
    QWidget* AddList(ols_property_t* prop, bool& warning);
    void AddEditableList(ols_property_t* prop, QFormLayout* layout,
        QLabel*& label);
    QWidget* AddButton(ols_property_t* prop);
    void AddColorInternal(ols_property_t* prop, QFormLayout* layout,
        QLabel*& label, bool supportAlpha);
    void AddColor(ols_property_t* prop, QFormLayout* layout,
        QLabel*& label);

    void AddColorAlpha(ols_property_t* prop, QFormLayout* layout,
        QLabel*& label);

    void AddFont(ols_property_t* prop, QFormLayout* layout, QLabel*& label);

    void AddGroup(ols_property_t* prop, QFormLayout* layout);

    void AddProperty(ols_property_t* property, QFormLayout* layout);

    void resizeEvent(QResizeEvent* event) override;

    void GetScrollPos(int& h, int& v);
    void SetScrollPos(int h, int v);

public slots:
    void ReloadProperties();
    void RefreshProperties();
    void SignalChanged();

signals:
    void PropertiesResized();
    void Changed();
    void PropertiesRefreshed();

public:
    OLSPropertiesView(OLSData settings, ols_object_t* obj,
        PropertiesReloadCallback reloadCallback,
        PropertiesUpdateCallback callback,
        PropertiesVisualUpdateCb cb = nullptr,
        int minSize = 0);
    OLSPropertiesView(OLSData settings, void* obj,
        PropertiesReloadCallback reloadCallback,
        PropertiesUpdateCallback callback,
        PropertiesVisualUpdateCb cb = nullptr,
        int minSize = 0);
    OLSPropertiesView(OLSData settings, const char* type,
        PropertiesReloadCallback reloadCallback,
        int minSize = 0);

#define obj_constructor(type)                                        \
    inline OLSPropertiesView(OLSData settings, ols_##type##_t* type, \
        PropertiesReloadCallback reloadCallback,                     \
        PropertiesUpdateCallback callback,                           \
        PropertiesVisualUpdateCb cb = nullptr,                       \
        int minSize = 0)                                             \
        : OLSPropertiesView(settings, (ols_object_t*)type,           \
            reloadCallback, callback, cb, minSize)                   \
    {                                                                \
    }

    obj_constructor(source);
    obj_constructor(output);
    //obj_constructor(service);
#undef obj_constructor

    inline ols_data_t* GetSettings() const
    {
        return settings;
    }

    inline void UpdateSettings()
    {
        if (callback)
            callback(OLSGetStrongRef(weakObj), nullptr, settings);
        else if (visUpdateCb)
            visUpdateCb(OLSGetStrongRef(weakObj), settings);
    }
    inline bool DeferUpdate() const { return deferUpdate; }

    inline OLSObject GetObject() const { return OLSGetStrongRef(weakObj); }

#define Def_IsObject(type)                           \
    inline bool IsObject(ols_##type##_t* type) const \
    {                                                \
        OLSObject obj = OLSGetStrongRef(weakObj);    \
        return obj.Get() == (ols_object_t*)type;     \
    }

    /* clang-format off */
    Def_IsObject(source)
    Def_IsObject(output)

    /* clang-format on */

#undef Def_IsObject
};
