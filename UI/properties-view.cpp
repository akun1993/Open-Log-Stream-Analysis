#include "properties-view.h"
#include "double-slider.h"
#include "plain-text-edit.h"
#include "qt-wrappers.h"
#include "slider-ignorewheel.h"
#include "spinbox-ignorewheel.h"
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFont>
#include <QFontDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStandardItem>
#include <cstdlib>
#include <initializer_list>
#include <math.h>
#include <ols-data.h>
#include <ols.h>
#include <qtimer.h>
#include <string>

#include "olsapp.h"

using namespace std;

static inline QColor color_from_int(long long val)
{
    return QColor(val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff,
        (val >> 24) & 0xff);
}

static inline long long color_to_int(QColor color)
{
    auto shift = [&](unsigned val, int shift) {
        return ((val & 0xff) << shift);
    };

    return shift(color.red(), 0) | shift(color.green(), 8) | shift(color.blue(), 16) | shift(color.alpha(), 24);
}

namespace {

struct frame_rate_tag {
    enum tag_type {
        SIMPLE,
        RATIONAL,
        USER,
    } type
        = SIMPLE;
    const char* val = nullptr;

    frame_rate_tag() = default;

    explicit frame_rate_tag(tag_type type)
        : type(type)
    {
    }

    explicit frame_rate_tag(const char* val)
        : type(USER)
        , val(val)
    {
    }

    static frame_rate_tag simple() { return frame_rate_tag { SIMPLE }; }
    static frame_rate_tag rational() { return frame_rate_tag { RATIONAL }; }
};
}

Q_DECLARE_METATYPE(frame_rate_tag);

void OLSPropertiesView::ReloadProperties()
{
    if (weakObj || rawObj) {
        OLSObject strongObj = GetObject();
        void* obj = strongObj ? strongObj.Get() : rawObj;
        if (obj)
            properties.reset(reloadCallback(obj));
    } else {
        properties.reset(reloadCallback((void*)type.c_str()));
        ols_properties_apply_settings(properties.get(), settings);
    }

    uint32_t flags = ols_properties_get_flags(properties.get());
    deferUpdate = (flags & OLS_PROPERTIES_DEFER_UPDATE) != 0;

    RefreshProperties();
}

#define NO_PROPERTIES_STRING tr("Basic.PropertiesWindow.NoProperties")

void OLSPropertiesView::RefreshProperties()
{
    int h, v;
    GetScrollPos(h, v);

    children.clear();
    if (widget)
        widget->deleteLater();

    widget = new QWidget();
    widget->setObjectName("PropertiesContainer");

    QFormLayout* layout = new QFormLayout;
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    widget->setLayout(layout);

    QSizePolicy mainPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    layout->setLabelAlignment(Qt::AlignRight);

    ols_property_t* property = ols_properties_first(properties.get());
    bool hasNoProperties = !property;

    while (property) {
        AddProperty(property, layout);
        ols_property_next(&property);
    }

    setWidgetResizable(true);
    setWidget(widget);
    SetScrollPos(h, v);
    setSizePolicy(mainPolicy);

    lastFocused.clear();
    if (lastWidget) {
        lastWidget->setFocus(Qt::OtherFocusReason);
        lastWidget = nullptr;
    }

    if (hasNoProperties) {
        QLabel* noPropertiesLabel = new QLabel(NO_PROPERTIES_STRING);
        layout->addWidget(noPropertiesLabel);
    }

    emit PropertiesRefreshed();
}

void OLSPropertiesView::SetScrollPos(int h, int v)
{
    QScrollBar* scroll = horizontalScrollBar();
    if (scroll)
        scroll->setValue(h);

    scroll = verticalScrollBar();
    if (scroll)
        scroll->setValue(v);
}

void OLSPropertiesView::GetScrollPos(int& h, int& v)
{
    h = v = 0;

    QScrollBar* scroll = horizontalScrollBar();
    if (scroll)
        h = scroll->value();

    scroll = verticalScrollBar();
    if (scroll)
        v = scroll->value();
}

OLSPropertiesView::OLSPropertiesView(OLSData settings_, ols_object_t* obj,
    PropertiesReloadCallback reloadCallback,
    PropertiesUpdateCallback callback_,
    PropertiesVisualUpdateCb visUpdateCb_,
    int minSize_)
    : VScrollArea(nullptr)
    , properties(nullptr, ols_properties_destroy)
    , settings(settings_)
    , weakObj(ols_object_get_weak_object(obj))
    , reloadCallback(reloadCallback)
    , callback(callback_)
    , visUpdateCb(visUpdateCb_)
    , minSize(minSize_)
{
    setFrameShape(QFrame::NoFrame);
    QMetaObject::invokeMethod(this, "ReloadProperties",
        Qt::QueuedConnection);
}

OLSPropertiesView::OLSPropertiesView(OLSData settings_, void* obj,
    PropertiesReloadCallback reloadCallback,
    PropertiesUpdateCallback callback_,
    PropertiesVisualUpdateCb visUpdateCb_,
    int minSize_)
    : VScrollArea(nullptr)
    , properties(nullptr, ols_properties_destroy)
    , settings(settings_)
    , rawObj(obj)
    , reloadCallback(reloadCallback)
    , callback(callback_)
    , visUpdateCb(visUpdateCb_)
    , minSize(minSize_)
{
    setFrameShape(QFrame::NoFrame);
    QMetaObject::invokeMethod(this, "ReloadProperties",
        Qt::QueuedConnection);
}

OLSPropertiesView::OLSPropertiesView(OLSData settings_, const char* type_,
    PropertiesReloadCallback reloadCallback_,
    int minSize_)
    : VScrollArea(nullptr)
    , properties(nullptr, ols_properties_destroy)
    , settings(settings_)
    , type(type_)
    , reloadCallback(reloadCallback_)
    , minSize(minSize_)
{
    setFrameShape(QFrame::NoFrame);
    QMetaObject::invokeMethod(this, "ReloadProperties",
        Qt::QueuedConnection);
}

void OLSPropertiesView::resizeEvent(QResizeEvent* event)
{
    emit PropertiesResized();
    VScrollArea::resizeEvent(event);
}

QWidget* OLSPropertiesView::NewWidget(ols_property_t* prop, QWidget* widget,
    const char* signal)
{
    const char* long_desc = ols_property_long_description(prop);

    WidgetInfo* info = new WidgetInfo(this, prop, widget);
    connect(widget, signal, info, SLOT(ControlChanged()));
    children.emplace_back(info);

    widget->setToolTip(QT_UTF8(long_desc));
    return widget;
}

QWidget* OLSPropertiesView::AddCheckbox(ols_property_t* prop)
{
    const char* name = ols_property_name(prop);
    const char* desc = ols_property_description(prop);
    bool val = ols_data_get_bool(settings, name);

    QCheckBox* checkbox = new QCheckBox(QT_UTF8(desc));
    checkbox->setCheckState(val ? Qt::Checked : Qt::Unchecked);
    return NewWidget(prop, checkbox, SIGNAL(stateChanged(int)));
}

QWidget* OLSPropertiesView::AddText(ols_property_t* prop, QFormLayout* layout,
    QLabel*& label)
{
    const char* name = ols_property_name(prop);
    const char* val = ols_data_get_string(settings, name);
    bool monospace = ols_property_text_monospace(prop);
    ols_text_type type = ols_property_text_type(prop);

    if (type == OLS_TEXT_MULTILINE) {
        OLSPlainTextEdit* edit = new OLSPlainTextEdit(this, monospace);
        edit->setPlainText(QT_UTF8(val));
        edit->setTabStopDistance(40);
        return NewWidget(prop, edit, SIGNAL(textChanged()));

    } else if (type == OLS_TEXT_PASSWORD) {
        QLayout* subLayout = new QHBoxLayout();
        QLineEdit* edit = new QLineEdit();
        QPushButton* show = new QPushButton();

        show->setText(tr("Show"));
        show->setCheckable(true);
        edit->setText(QT_UTF8(val));
        edit->setEchoMode(QLineEdit::Password);

        subLayout->addWidget(edit);
        subLayout->addWidget(show);

        WidgetInfo* info = new WidgetInfo(this, prop, edit);
        connect(show, &QAbstractButton::toggled, info,
            &WidgetInfo::TogglePasswordText);
        connect(show, &QAbstractButton::toggled, [=](bool hide) {
            show->setText(hide ? tr("Hide") : tr("Show"));
        });
        children.emplace_back(info);

        label = new QLabel(QT_UTF8(ols_property_description(prop)));
        layout->addRow(label, subLayout);

        edit->setToolTip(QT_UTF8(ols_property_long_description(prop)));

        connect(edit, SIGNAL(textEdited(const QString&)), info,
            SLOT(ControlChanged()));
        return nullptr;
    } else if (type == OLS_TEXT_INFO) {
        QString desc = QT_UTF8(ols_property_description(prop));
        const char* long_desc = ols_property_long_description(prop);
        ols_text_info_type info_type = ols_property_text_info_type(prop);

        QLabel* info_label = new QLabel(QT_UTF8(val));

        if (info_label->text().isEmpty() && long_desc == NULL) {
            label = nullptr;
            info_label->setText(desc);
        } else
            label = new QLabel(desc);

        if (long_desc != NULL && !info_label->text().isEmpty()) {
            bool themeDark = false; //TODO !App()->IsThemeDark();
            QString file = themeDark
                ? ":/res/images/help.svg"
                : ":/res/images/help_light.svg";
            QString lStr = "<html>%1 <img src='%2' style=' \
                vertical-align: bottom; ' /></html>";

            info_label->setText(lStr.arg(info_label->text(), file));
            info_label->setToolTip(QT_UTF8(long_desc));

        } else if (long_desc != NULL) {
            info_label->setText(QT_UTF8(long_desc));
        }

        info_label->setOpenExternalLinks(true);
        info_label->setWordWrap(ols_property_text_info_word_wrap(prop));

        if (info_type == OLS_TEXT_INFO_WARNING)
            info_label->setObjectName("warningLabel");
        else if (info_type == OLS_TEXT_INFO_ERROR)
            info_label->setObjectName("errorLabel");

        if (label)
            label->setObjectName(info_label->objectName());

        WidgetInfo* info = new WidgetInfo(this, prop, info_label);
        children.emplace_back(info);

        layout->addRow(label, info_label);

        return nullptr;
    }

    QLineEdit* edit = new QLineEdit();

    edit->setText(QT_UTF8(val));
    edit->setToolTip(QT_UTF8(ols_property_long_description(prop)));

    return NewWidget(prop, edit, SIGNAL(textEdited(const QString&)));
}

void OLSPropertiesView::AddPath(ols_property_t* prop, QFormLayout* layout,
    QLabel** label)
{
    const char* name = ols_property_name(prop);
    const char* val = ols_data_get_string(settings, name);
    QLayout* subLayout = new QHBoxLayout();
    QLineEdit* edit = new QLineEdit();
    QPushButton* button = new QPushButton(tr("Browse"));

    if (!ols_property_enabled(prop)) {
        edit->setEnabled(false);
        button->setEnabled(false);
    }

    button->setProperty("themeID", "settingsButtons");
    edit->setText(QT_UTF8(val));
    edit->setReadOnly(true);
    edit->setToolTip(QT_UTF8(ols_property_long_description(prop)));

    subLayout->addWidget(edit);
    subLayout->addWidget(button);

    WidgetInfo* info = new WidgetInfo(this, prop, edit);
    connect(button, SIGNAL(clicked()), info, SLOT(ControlChanged()));
    children.emplace_back(info);

    *label = new QLabel(QT_UTF8(ols_property_description(prop)));
    layout->addRow(*label, subLayout);
}

void OLSPropertiesView::AddInt(ols_property_t* prop, QFormLayout* layout,
    QLabel** label)
{
    ols_number_type type = ols_property_int_type(prop);
    QLayout* subLayout = new QHBoxLayout();

    const char* name = ols_property_name(prop);
    int val = (int)ols_data_get_int(settings, name);
    QSpinBox* spin = new SpinBoxIgnoreScroll();

    spin->setEnabled(ols_property_enabled(prop));

    int minVal = ols_property_int_min(prop);
    int maxVal = ols_property_int_max(prop);
    int stepVal = ols_property_int_step(prop);
    const char* suffix = ols_property_int_suffix(prop);

    spin->setMinimum(minVal);
    spin->setMaximum(maxVal);
    spin->setSingleStep(stepVal);
    spin->setValue(val);
    spin->setToolTip(QT_UTF8(ols_property_long_description(prop)));
    spin->setSuffix(QT_UTF8(suffix));

    WidgetInfo* info = new WidgetInfo(this, prop, spin);
    children.emplace_back(info);

    if (type == OLS_NUMBER_SLIDER) {
        QSlider* slider = new SliderIgnoreScroll();
        slider->setMinimum(minVal);
        slider->setMaximum(maxVal);
        slider->setPageStep(stepVal);
        slider->setValue(val);
        slider->setOrientation(Qt::Horizontal);
        slider->setEnabled(ols_property_enabled(prop));
        subLayout->addWidget(slider);

        connect(slider, SIGNAL(valueChanged(int)), spin,
            SLOT(setValue(int)));
        connect(spin, SIGNAL(valueChanged(int)), slider,
            SLOT(setValue(int)));
    }

    connect(spin, SIGNAL(valueChanged(int)), info, SLOT(ControlChanged()));

    subLayout->addWidget(spin);

    *label = new QLabel(QT_UTF8(ols_property_description(prop)));
    layout->addRow(*label, subLayout);
}

void OLSPropertiesView::AddFloat(ols_property_t* prop, QFormLayout* layout,
    QLabel** label)
{
    ols_number_type type = ols_property_float_type(prop);
    QLayout* subLayout = new QHBoxLayout();

    const char* name = ols_property_name(prop);
    double val = ols_data_get_double(settings, name);
    QDoubleSpinBox* spin = new QDoubleSpinBox();

    if (!ols_property_enabled(prop))
        spin->setEnabled(false);

    double minVal = ols_property_float_min(prop);
    double maxVal = ols_property_float_max(prop);
    double stepVal = ols_property_float_step(prop);
    const char* suffix = ols_property_float_suffix(prop);

    if (stepVal < 1.0) {
        constexpr int sane_limit = 8;
        const int decimals = std::min<int>(log10(1.0 / stepVal) + 0.99, sane_limit);
        if (decimals > spin->decimals())
            spin->setDecimals(decimals);
    }

    spin->setMinimum(minVal);
    spin->setMaximum(maxVal);
    spin->setSingleStep(stepVal);
    spin->setValue(val);
    spin->setToolTip(QT_UTF8(ols_property_long_description(prop)));
    spin->setSuffix(QT_UTF8(suffix));

    WidgetInfo* info = new WidgetInfo(this, prop, spin);
    children.emplace_back(info);

    if (type == OLS_NUMBER_SLIDER) {
        DoubleSlider* slider = new DoubleSlider();
        slider->setDoubleConstraints(minVal, maxVal, stepVal, val);
        slider->setOrientation(Qt::Horizontal);
        subLayout->addWidget(slider);

        connect(slider, SIGNAL(doubleValChanged(double)), spin,
            SLOT(setValue(double)));
        connect(spin, SIGNAL(valueChanged(double)), slider,
            SLOT(setDoubleVal(double)));
    }

    connect(spin, SIGNAL(valueChanged(double)), info,
        SLOT(ControlChanged()));

    subLayout->addWidget(spin);

    *label = new QLabel(QT_UTF8(ols_property_description(prop)));
    layout->addRow(*label, subLayout);
}

static void AddComboItem(QComboBox* combo, ols_property_t* prop,
    ols_combo_format format, size_t idx)
{
    const char* name = ols_property_list_item_name(prop, idx);
    QVariant var;

    if (format == OLS_COMBO_FORMAT_INT) {
        long long val = ols_property_list_item_int(prop, idx);
        var = QVariant::fromValue<long long>(val);

    } else if (format == OLS_COMBO_FORMAT_FLOAT) {
        double val = ols_property_list_item_float(prop, idx);
        var = QVariant::fromValue<double>(val);

    } else if (format == OLS_COMBO_FORMAT_STRING) {
        var = QByteArray(ols_property_list_item_string(prop, idx));
    }

    combo->addItem(QT_UTF8(name), var);

    if (!ols_property_list_item_disabled(prop, idx))
        return;

    int index = combo->findText(QT_UTF8(name));
    if (index < 0)
        return;

    QStandardItemModel* model = dynamic_cast<QStandardItemModel*>(combo->model());
    if (!model)
        return;

    QStandardItem* item = model->item(index);
    item->setFlags(Qt::NoItemFlags);
}

template <long long get_int(ols_data_t*, const char*),
    double get_double(ols_data_t*, const char*),
    const char* get_string(ols_data_t*, const char*)>
static QVariant from_ols_data(ols_data_t* data, const char* name,
    ols_combo_format format)
{
    switch (format) {
    case OLS_COMBO_FORMAT_INT:
        return QVariant::fromValue(get_int(data, name));
    case OLS_COMBO_FORMAT_FLOAT:
        return QVariant::fromValue(get_double(data, name));
    case OLS_COMBO_FORMAT_STRING:
        return QByteArray(get_string(data, name));
    default:
        return QVariant();
    }
}

static QVariant from_ols_data(ols_data_t* data, const char* name,
    ols_combo_format format)
{
    return from_ols_data<ols_data_get_int, ols_data_get_double,
        ols_data_get_string>(data, name, format);
}

static QVariant from_ols_data_autoselect(ols_data_t* data, const char* name,
    ols_combo_format format)
{
    return from_ols_data<ols_data_get_autoselect_int,
        ols_data_get_autoselect_double,
        ols_data_get_autoselect_string>(data, name,
        format);
}

QWidget* OLSPropertiesView::AddList(ols_property_t* prop, bool& warning)
{
    const char* name = ols_property_name(prop);
    QComboBox* combo = new QComboBox();
    ols_combo_type type = ols_property_list_type(prop);
    ols_combo_format format = ols_property_list_format(prop);
    size_t count = ols_property_list_item_count(prop);
    int idx = -1;

    for (size_t i = 0; i < count; i++)
        AddComboItem(combo, prop, format, i);

    if (type == OLS_COMBO_TYPE_EDITABLE)
        combo->setEditable(true);

    combo->setMaxVisibleItems(40);
    combo->setToolTip(QT_UTF8(ols_property_long_description(prop)));

    QVariant value = from_ols_data(settings, name, format);

    if (format == OLS_COMBO_FORMAT_STRING && type == OLS_COMBO_TYPE_EDITABLE) {
        combo->lineEdit()->setText(value.toString());
    } else {
        idx = combo->findData(value);
    }

    if (type == OLS_COMBO_TYPE_EDITABLE)
        return NewWidget(prop, combo,
            SIGNAL(editTextChanged(const QString&)));

    if (idx != -1)
        combo->setCurrentIndex(idx);

    if (ols_data_has_autoselect_value(settings, name)) {
        QVariant autoselect = from_ols_data_autoselect(settings, name, format);
        int id = combo->findData(autoselect);

        if (id != -1 && id != idx) {
            QString actual = combo->itemText(id);
            QString selected = combo->itemText(idx);
            QString combined = tr(
                "Basic.PropertiesWindow.AutoSelectFormat");
            combo->setItemText(idx,
                combined.arg(selected).arg(actual));
        }
    }

    QAbstractItemModel* model = combo->model();
    warning = idx != -1 && model->flags(model->index(idx, 0)) == Qt::NoItemFlags;

    WidgetInfo* info = new WidgetInfo(this, prop, combo);
    connect(combo, SIGNAL(currentIndexChanged(int)), info,
        SLOT(ControlChanged()));
    children.emplace_back(info);

    /* trigger a settings update if the index was not found */
    if (idx == -1)
        info->ControlChanged();

    return combo;
}

static void NewButton(QLayout* layout, WidgetInfo* info, const char* themeIcon,
    void (WidgetInfo::*method)())
{
    QPushButton* button = new QPushButton();
    button->setProperty("themeID", themeIcon);
    button->setFlat(true);
    button->setProperty("toolButton", true);

    QObject::connect(button, &QPushButton::clicked, info, method);

    layout->addWidget(button);
}

void OLSPropertiesView::AddEditableList(ols_property_t* prop,
    QFormLayout* layout, QLabel*& label)
{
    const char* name = ols_property_name(prop);
    OLSDataArrayAutoRelease array = ols_data_get_array(settings, name);
    QListWidget* list = new QListWidget();
    size_t count = ols_data_array_count(array);

    if (!ols_property_enabled(prop))
        list->setEnabled(false);

    list->setSortingEnabled(false);
    list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    list->setToolTip(QT_UTF8(ols_property_long_description(prop)));
    list->setSpacing(1);

    for (size_t i = 0; i < count; i++) {
        OLSDataAutoRelease item = ols_data_array_item(array, i);
        list->addItem(QT_UTF8(ols_data_get_string(item, "value")));
        QListWidgetItem* const list_item = list->item((int)i);
        list_item->setSelected(ols_data_get_bool(item, "selected"));
        list_item->setHidden(ols_data_get_bool(item, "hidden"));
    }

    WidgetInfo* info = new WidgetInfo(this, prop, list);

    list->setDragDropMode(QAbstractItemView::InternalMove);
    connect(list->model(),
        SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int)),
        info,
        SLOT(EditListReordered(const QModelIndex&, int, int,
            const QModelIndex&, int)));

    QVBoxLayout* sideLayout = new QVBoxLayout();
    NewButton(sideLayout, info, "addIconSmall", &WidgetInfo::EditListAdd);
    NewButton(sideLayout, info, "removeIconSmall",
        &WidgetInfo::EditListRemove);
    NewButton(sideLayout, info, "configIconSmall",
        &WidgetInfo::EditListEdit);
    NewButton(sideLayout, info, "upArrowIconSmall",
        &WidgetInfo::EditListUp);
    NewButton(sideLayout, info, "downArrowIconSmall",
        &WidgetInfo::EditListDown);
    sideLayout->addStretch(0);

    QHBoxLayout* subLayout = new QHBoxLayout();
    subLayout->addWidget(list);
    subLayout->addLayout(sideLayout);

    children.emplace_back(info);

    label = new QLabel(QT_UTF8(ols_property_description(prop)));
    layout->addRow(label, subLayout);
}

QWidget* OLSPropertiesView::AddButton(ols_property_t* prop)
{
    const char* desc = ols_property_description(prop);

    QPushButton* button = new QPushButton(QT_UTF8(desc));
    button->setProperty("themeID", "settingsButtons");
    button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    return NewWidget(prop, button, SIGNAL(clicked()));
}

void OLSPropertiesView::AddColorInternal(ols_property_t* prop,
    QFormLayout* layout, QLabel*& label,
    bool supportAlpha)
{
    QPushButton* button = new QPushButton;
    QLabel* colorLabel = new QLabel;
    const char* name = ols_property_name(prop);
    long long val = ols_data_get_int(settings, name);
    QColor color = color_from_int(val);
    QColor::NameFormat format;

    if (!ols_property_enabled(prop)) {
        button->setEnabled(false);
        colorLabel->setEnabled(false);
    }

    button->setProperty("themeID", "settingsButtons");
    button->setText(tr("Basic.PropertiesWindow.SelectColor"));
    button->setToolTip(QT_UTF8(ols_property_long_description(prop)));

    if (supportAlpha) {
        format = QColor::HexArgb;
    } else {
        format = QColor::HexRgb;
        color.setAlpha(255);
    }

    QPalette palette = QPalette(color);
    colorLabel->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    colorLabel->setText(color.name(format));
    colorLabel->setPalette(palette);
    colorLabel->setStyleSheet(
        QString("background-color :%1; color: %2;")
            .arg(palette.color(QPalette::Window).name(format))
            .arg(palette.color(QPalette::WindowText).name(format)));
    colorLabel->setAutoFillBackground(true);
    colorLabel->setAlignment(Qt::AlignCenter);
    colorLabel->setToolTip(QT_UTF8(ols_property_long_description(prop)));

    QHBoxLayout* subLayout = new QHBoxLayout;
    subLayout->setContentsMargins(0, 0, 0, 0);

    subLayout->addWidget(colorLabel);
    subLayout->addWidget(button);

    WidgetInfo* info = new WidgetInfo(this, prop, colorLabel);
    connect(button, SIGNAL(clicked()), info, SLOT(ControlChanged()));
    children.emplace_back(info);

    label = new QLabel(QT_UTF8(ols_property_description(prop)));
    layout->addRow(label, subLayout);
}

void OLSPropertiesView::AddColor(ols_property_t* prop, QFormLayout* layout,
    QLabel*& label)
{
    AddColorInternal(prop, layout, label, false);
}

void OLSPropertiesView::AddColorAlpha(ols_property_t* prop, QFormLayout* layout,
    QLabel*& label)
{
    AddColorInternal(prop, layout, label, true);
}

void MakeQFont(ols_data_t* font_obj, QFont& font, bool limit = false)
{
    const char* face = ols_data_get_string(font_obj, "face");
    const char* style = ols_data_get_string(font_obj, "style");
    int size = (int)ols_data_get_int(font_obj, "size");
    uint32_t flags = (uint32_t)ols_data_get_int(font_obj, "flags");

    if (face) {
        font.setFamily(face);
        font.setStyleName(style);
    }

    if (size) {
        if (limit) {
            int max_size = font.pointSize();
            if (max_size < 28)
                max_size = 28;
            if (size > max_size)
                size = max_size;
        }
        font.setPointSize(size);
    }

    if (flags & OLS_FONT_BOLD)
        font.setBold(true);
    if (flags & OLS_FONT_ITALIC)
        font.setItalic(true);
    if (flags & OLS_FONT_UNDERLINE)
        font.setUnderline(true);
    if (flags & OLS_FONT_STRIKEOUT)
        font.setStrikeOut(true);
}

void OLSPropertiesView::AddFont(ols_property_t* prop, QFormLayout* layout,
    QLabel*& label)
{
    const char* name = ols_property_name(prop);
    OLSDataAutoRelease font_obj = ols_data_get_obj(settings, name);
    const char* face = ols_data_get_string(font_obj, "face");
    const char* style = ols_data_get_string(font_obj, "style");
    QPushButton* button = new QPushButton;
    QLabel* fontLabel = new QLabel;
    QFont font;

    if (!ols_property_enabled(prop)) {
        button->setEnabled(false);
        fontLabel->setEnabled(false);
    }

    font = fontLabel->font();
    MakeQFont(font_obj, font, true);

    button->setProperty("themeID", "settingsButtons");
    button->setText(tr("Basic.PropertiesWindow.SelectFont"));
    button->setToolTip(QT_UTF8(ols_property_long_description(prop)));

    fontLabel->setFrameStyle(QFrame::Sunken | QFrame::Panel);
    fontLabel->setFont(font);
    fontLabel->setText(QString("%1 %2").arg(face, style));
    fontLabel->setAlignment(Qt::AlignCenter);
    fontLabel->setToolTip(QT_UTF8(ols_property_long_description(prop)));

    QHBoxLayout* subLayout = new QHBoxLayout;
    subLayout->setContentsMargins(0, 0, 0, 0);

    subLayout->addWidget(fontLabel);
    subLayout->addWidget(button);

    WidgetInfo* info = new WidgetInfo(this, prop, fontLabel);
    connect(button, SIGNAL(clicked()), info, SLOT(ControlChanged()));
    children.emplace_back(info);

    label = new QLabel(QT_UTF8(ols_property_description(prop)));
    layout->addRow(label, subLayout);
}

namespace std {

template <>
struct default_delete<ols_data_t> {
    void operator()(ols_data_t* data) { ols_data_release(data); }
};

template <>
struct default_delete<ols_data_item_t> {
    void operator()(ols_data_item_t* item) { ols_data_item_release(&item); }
};

}

template <typename T>
static double make_epsilon(T val)
{
    return val * 0.00001;
}

void OLSPropertiesView::AddGroup(ols_property_t* prop, QFormLayout* layout)
{
    const char* name = ols_property_name(prop);
    bool val = ols_data_get_bool(settings, name);
    const char* desc = ols_property_description(prop);
    enum ols_group_type type = ols_property_group_type(prop);

    // Create GroupBox
    QGroupBox* groupBox = new QGroupBox(QT_UTF8(desc));
    groupBox->setCheckable(type == OLS_GROUP_CHECKABLE);
    groupBox->setChecked(groupBox->isCheckable() ? val : true);
    groupBox->setAccessibleName("group");
    groupBox->setEnabled(ols_property_enabled(prop));

    // Create Layout and build content
    QFormLayout* subLayout = new QFormLayout();
    subLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    groupBox->setLayout(subLayout);

    ols_properties_t* content = ols_property_group_content(prop);
    ols_property_t* el = ols_properties_first(content);
    while (el != nullptr) {
        AddProperty(el, subLayout);
        ols_property_next(&el);
    }

    // Insert into UI
    layout->setWidget(layout->rowCount(),
        QFormLayout::ItemRole::SpanningRole, groupBox);

    // Register Group Widget
    WidgetInfo* info = new WidgetInfo(this, prop, groupBox);
    children.emplace_back(info);

    // Signals
    connect(groupBox, SIGNAL(toggled(bool)), info, SLOT(ControlChanged()));
}

void OLSPropertiesView::AddProperty(ols_property_t* property,
    QFormLayout* layout)
{
    const char* name = ols_property_name(property);
    ols_property_type type = ols_property_get_type(property);

    if (!ols_property_visible(property))
        return;

    QLabel* label = nullptr;
    QWidget* widget = nullptr;
    bool warning = false;

    switch (type) {
    case OLS_PROPERTY_INVALID:
        return;
    case OLS_PROPERTY_BOOL:
        widget = AddCheckbox(property);
        break;
    case OLS_PROPERTY_INT:
        AddInt(property, layout, &label);
        break;
    case OLS_PROPERTY_FLOAT:
        AddFloat(property, layout, &label);
        break;
    case OLS_PROPERTY_TEXT:
        widget = AddText(property, layout, label);
        break;
    case OLS_PROPERTY_PATH:
        AddPath(property, layout, &label);
        break;
    case OLS_PROPERTY_LIST:
        widget = AddList(property, warning);
        break;
    case OLS_PROPERTY_COLOR:
        AddColor(property, layout, label);
        break;
    case OLS_PROPERTY_FONT:
        AddFont(property, layout, label);
        break;
    case OLS_PROPERTY_BUTTON:
        widget = AddButton(property);
        break;
    case OLS_PROPERTY_EDITABLE_LIST:
        AddEditableList(property, layout, label);
        break;

    case OLS_PROPERTY_GROUP:
        AddGroup(property, layout);
        break;
    case OLS_PROPERTY_COLOR_ALPHA:
        AddColorAlpha(property, layout, label);
    }

    if (!widget && !label)
        return;

    if (!label && type != OLS_PROPERTY_BOOL && type != OLS_PROPERTY_BUTTON && type != OLS_PROPERTY_GROUP)
        label = new QLabel(QT_UTF8(ols_property_description(property)));

    if (label) {
        if (warning) //TODO: select color based on background color
            label->setStyleSheet("QLabel { color: red; }");

        if (minSize) {
            label->setMinimumWidth(minSize);
            label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        }

        if (!ols_property_enabled(property))
            label->setEnabled(false);
    }

    if (!widget)
        return;

    if (!ols_property_enabled(property))
        widget->setEnabled(false);

    if (ols_property_long_description(property)) {
        bool themeDark = false; //App()->IsThemeDark();
        QString file = !themeDark
            ? ":/res/images/help.svg"
            : ":/res/images/help_light.svg";
        if (label) {
            QString lStr = "<html>%1 <img src='%2' style=' \
                vertical-align: bottom;  \
                ' /></html>";

            label->setText(lStr.arg(label->text(), file));
            label->setToolTip(
                ols_property_long_description(property));
        } else if (type == OLS_PROPERTY_BOOL) {

            QString bStr = "<html> <img src='%1' style=' \
                vertical-align: bottom;  \
                ' /></html>";

            const char* desc = ols_property_description(property);

            QWidget* newWidget = new QWidget();

            QHBoxLayout* boxLayout = new QHBoxLayout(newWidget);
            boxLayout->setContentsMargins(0, 0, 0, 0);
            boxLayout->setAlignment(Qt::AlignLeft);
            boxLayout->setSpacing(0);

            QCheckBox* check = qobject_cast<QCheckBox*>(widget);
            check->setText(desc);
            check->setToolTip(
                ols_property_long_description(property));

            QLabel* help = new QLabel(check);
            help->setText(bStr.arg(file));
            help->setToolTip(
                ols_property_long_description(property));

            boxLayout->addWidget(check);
            boxLayout->addWidget(help);
            widget = newWidget;
        }
    }

    layout->addRow(label, widget);

    if (!lastFocused.empty())
        if (lastFocused.compare(name) == 0)
            lastWidget = widget;
}

void OLSPropertiesView::SignalChanged()
{
    emit Changed();
}

void WidgetInfo::BoolChanged(const char* setting)
{
    QCheckBox* checkbox = static_cast<QCheckBox*>(widget);
    ols_data_set_bool(view->settings, setting,
        checkbox->checkState() == Qt::Checked);
}

void WidgetInfo::IntChanged(const char* setting)
{
    QSpinBox* spin = static_cast<QSpinBox*>(widget);
    ols_data_set_int(view->settings, setting, spin->value());
}

void WidgetInfo::FloatChanged(const char* setting)
{
    QDoubleSpinBox* spin = static_cast<QDoubleSpinBox*>(widget);
    ols_data_set_double(view->settings, setting, spin->value());
}

void WidgetInfo::TextChanged(const char* setting)
{
    ols_text_type type = ols_property_text_type(property);

    if (type == OLS_TEXT_MULTILINE) {
        OLSPlainTextEdit* edit = static_cast<OLSPlainTextEdit*>(widget);
        ols_data_set_string(view->settings, setting,
            QT_TO_UTF8(edit->toPlainText()));
        return;
    }

    QLineEdit* edit = static_cast<QLineEdit*>(widget);
    ols_data_set_string(view->settings, setting, QT_TO_UTF8(edit->text()));
}

bool WidgetInfo::PathChanged(const char* setting)
{
    const char* desc = ols_property_description(property);
    ols_path_type type = ols_property_path_type(property);
    const char* filter = ols_property_path_filter(property);
    const char* default_path = ols_property_path_default_path(property);
    QString path;

    if (type == OLS_PATH_DIRECTORY)
        path = SelectDirectory(view, QT_UTF8(desc),
            QT_UTF8(default_path));
    else if (type == OLS_PATH_FILE)
        path = OpenFile(view, QT_UTF8(desc), QT_UTF8(default_path),
            QT_UTF8(filter));
    else if (type == OLS_PATH_FILE_SAVE)
        path = SaveFile(view, QT_UTF8(desc), QT_UTF8(default_path),
            QT_UTF8(filter));

    if (path.isEmpty())
        return false;

    QLineEdit* edit = static_cast<QLineEdit*>(widget);
    edit->setText(path);
    ols_data_set_string(view->settings, setting, QT_TO_UTF8(path));
    return true;
}

void WidgetInfo::ListChanged(const char* setting)
{
    QComboBox* combo = static_cast<QComboBox*>(widget);
    ols_combo_format format = ols_property_list_format(property);
    ols_combo_type type = ols_property_list_type(property);
    QVariant data;

    if (type == OLS_COMBO_TYPE_EDITABLE) {
        data = combo->currentText().toUtf8();
    } else {
        int index = combo->currentIndex();
        if (index != -1)
            data = combo->itemData(index);
        else
            return;
    }

    switch (format) {
    case OLS_COMBO_FORMAT_INVALID:
        return;
    case OLS_COMBO_FORMAT_INT:
        ols_data_set_int(view->settings, setting,
            data.value<long long>());
        break;
    case OLS_COMBO_FORMAT_FLOAT:
        ols_data_set_double(view->settings, setting,
            data.value<double>());
        break;
    case OLS_COMBO_FORMAT_STRING:
        ols_data_set_string(view->settings, setting,
            data.toByteArray().constData());
        break;
    }
}

bool WidgetInfo::ColorChangedInternal(const char* setting, bool supportAlpha)
{
    const char* desc = ols_property_description(property);
    long long val = ols_data_get_int(view->settings, setting);
    QColor color = color_from_int(val);
    QColor::NameFormat format;

    QColorDialog::ColorDialogOptions options;

    if (supportAlpha) {
        options |= QColorDialog::ShowAlphaChannel;
    }

    /* The native dialog on OSX has all kinds of problems, like closing
     * other open QDialogs on exit, and
     * https://bugreports.qt-project.org/browse/QTBUG-34532
     */
#ifndef _WIN32
    options |= QColorDialog::DontUseNativeDialog;
#endif

    color = QColorDialog::getColor(color, view, QT_UTF8(desc), options);
    if (!color.isValid())
        return false;

    if (supportAlpha) {
        format = QColor::HexArgb;
    } else {
        color.setAlpha(255);
        format = QColor::HexRgb;
    }

    QLabel* label = static_cast<QLabel*>(widget);
    label->setText(color.name(format));
    QPalette palette = QPalette(color);
    label->setPalette(palette);
    label->setStyleSheet(
        QString("background-color :%1; color: %2;")
            .arg(palette.color(QPalette::Window).name(format))
            .arg(palette.color(QPalette::WindowText).name(format)));

    ols_data_set_int(view->settings, setting, color_to_int(color));

    return true;
}

bool WidgetInfo::ColorChanged(const char* setting)
{
    return ColorChangedInternal(setting, false);
}

bool WidgetInfo::ColorAlphaChanged(const char* setting)
{
    return ColorChangedInternal(setting, true);
}

bool WidgetInfo::FontChanged(const char* setting)
{
    OLSDataAutoRelease font_obj = ols_data_get_obj(view->settings, setting);
    bool success;
    uint32_t flags;
    QFont font;

    QFontDialog::FontDialogOptions options;

#ifndef _WIN32
    options = QFontDialog::DontUseNativeDialog;
#endif

    if (!font_obj) {
        QFont initial;
        font = QFontDialog::getFont(&success, initial, view,
            "Pick a Font", options);
    } else {
        MakeQFont(font_obj, font);
        font = QFontDialog::getFont(&success, font, view, "Pick a Font",
            options);
    }

    if (!success)
        return false;

    font_obj = ols_data_create();

    ols_data_set_string(font_obj, "face", QT_TO_UTF8(font.family()));
    ols_data_set_string(font_obj, "style", QT_TO_UTF8(font.styleName()));
    ols_data_set_int(font_obj, "size", font.pointSize());
    flags = font.bold() ? OLS_FONT_BOLD : 0;
    flags |= font.italic() ? OLS_FONT_ITALIC : 0;
    flags |= font.underline() ? OLS_FONT_UNDERLINE : 0;
    flags |= font.strikeOut() ? OLS_FONT_STRIKEOUT : 0;
    ols_data_set_int(font_obj, "flags", flags);

    QLabel* label = static_cast<QLabel*>(widget);
    QFont labelFont;
    MakeQFont(font_obj, labelFont, true);
    label->setFont(labelFont);
    label->setText(QString("%1 %2").arg(font.family(), font.styleName()));

    ols_data_set_obj(view->settings, setting, font_obj);
    return true;
}

void WidgetInfo::GroupChanged(const char* setting)
{
    QGroupBox* groupbox = static_cast<QGroupBox*>(widget);
    ols_data_set_bool(view->settings, setting,
        groupbox->isCheckable() ? groupbox->isChecked()
                                : true);
}

void WidgetInfo::EditListReordered(const QModelIndex& parent, int start,
    int end, const QModelIndex& destination,
    int row)
{
    UNUSED_PARAMETER(parent);
    UNUSED_PARAMETER(start);
    UNUSED_PARAMETER(end);
    UNUSED_PARAMETER(destination);
    UNUSED_PARAMETER(row);

    EditableListChanged();
}

void WidgetInfo::EditableListChanged()
{
    const char* setting = ols_property_name(property);
    QListWidget* list = reinterpret_cast<QListWidget*>(widget);
    OLSDataArrayAutoRelease array = ols_data_array_create();

    for (int i = 0; i < list->count(); i++) {
        QListWidgetItem* item = list->item(i);
        OLSDataAutoRelease arrayItem = ols_data_create();
        ols_data_set_string(arrayItem, "value",
            QT_TO_UTF8(item->text()));
        ols_data_set_bool(arrayItem, "selected", item->isSelected());
        ols_data_set_bool(arrayItem, "hidden", item->isHidden());
        ols_data_array_push_back(array, arrayItem);
    }

    ols_data_set_array(view->settings, setting, array);

    ControlChanged();
}

void WidgetInfo::ButtonClicked()
{
    ols_button_type type = ols_property_button_type(property);
    const char* savedUrl = ols_property_button_url(property);

    if (type == OLS_BUTTON_URL && strcmp(savedUrl, "") != 0) {
        QUrl url(savedUrl, QUrl::StrictMode);
        if (url.isValid() && (url.scheme().compare("http") == 0 || url.scheme().compare("https") == 0)) {
            QString msg(
                tr("Basic.PropertiesView.UrlButton.Text"));
            msg += "\n\n";
            msg += QString(tr("Basic.PropertiesView.UrlButton.Text.Url"))
                       .arg(savedUrl);

            QMessageBox::StandardButton button = OBSMessageBox::question(
                view->window(),
                tr("Basic.PropertiesView.UrlButton.OpenUrl"),
                msg, QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

            if (button == QMessageBox::Yes)
                QDesktopServices::openUrl(url);
        }
        return;
    }
    if (view->rawObj || view->weakObj) {
        OLSObject strongObj = view->GetObject();
        void* obj = strongObj ? strongObj.Get() : view->rawObj;
        if (ols_property_button_clicked(property, obj)) {
            QMetaObject::invokeMethod(view, "RefreshProperties",
                Qt::QueuedConnection);
        }
    }
}

void WidgetInfo::TogglePasswordText(bool show)
{
    reinterpret_cast<QLineEdit*>(widget)->setEchoMode(
        show ? QLineEdit::Normal : QLineEdit::Password);
}

void WidgetInfo::ControlChanged()
{
    const char* setting = ols_property_name(property);
    ols_property_type type = ols_property_get_type(property);

    if (!recently_updated) {
        old_settings_cache = ols_data_create();
        ols_data_apply(old_settings_cache, view->settings);
        ols_data_release(old_settings_cache);
    }

    switch (type) {
    case OLS_PROPERTY_INVALID:
        return;
    case OLS_PROPERTY_BOOL:
        BoolChanged(setting);
        break;
    case OLS_PROPERTY_INT:
        IntChanged(setting);
        break;
    case OLS_PROPERTY_FLOAT:
        FloatChanged(setting);
        break;
    case OLS_PROPERTY_TEXT:
        TextChanged(setting);
        break;
    case OLS_PROPERTY_LIST:
        ListChanged(setting);
        break;
    case OLS_PROPERTY_BUTTON:
        ButtonClicked();
        return;
    case OLS_PROPERTY_COLOR:
        if (!ColorChanged(setting))
            return;
        break;
    case OLS_PROPERTY_FONT:
        if (!FontChanged(setting))
            return;
        break;
    case OLS_PROPERTY_PATH:
        if (!PathChanged(setting))
            return;
        break;
    case OLS_PROPERTY_EDITABLE_LIST:
        break;

    case OLS_PROPERTY_GROUP:
        GroupChanged(setting);
        break;
    case OLS_PROPERTY_COLOR_ALPHA:
        if (!ColorAlphaChanged(setting))
            return;
        break;
    }

    if (!recently_updated) {
        recently_updated = true;
        update_timer = new QTimer;
        connect(update_timer, &QTimer::timeout,
            [this, &ru = recently_updated]() {
                OLSObject strongObj = view->GetObject();
                void* obj = strongObj ? strongObj.Get()
                                      : view->rawObj;
                if (obj && view->callback && !view->deferUpdate) {
                    view->callback(obj, old_settings_cache,
                        view->settings);
                }

                ru = false;
            });
        connect(update_timer, &QTimer::timeout, &QTimer::deleteLater);
        update_timer->setSingleShot(true);
    }

    if (update_timer) {
        update_timer->stop();
        update_timer->start(500);
    } else {
        blog(LOG_DEBUG, "No update timer or no callback!");
    }

    if (view->visUpdateCb && !view->deferUpdate) {
        OLSObject strongObj = view->GetObject();
        void* obj = strongObj ? strongObj.Get() : view->rawObj;
        if (obj)
            view->visUpdateCb(obj, view->settings);
    }

    view->SignalChanged();

    if (ols_property_modified(property, view->settings)) {
        view->lastFocused = setting;
        QMetaObject::invokeMethod(view, "RefreshProperties",
            Qt::QueuedConnection);
    }
}

class EditableItemDialog : public QDialog {
    QLineEdit* edit;
    QString filter;
    QString default_path;

    void BrowseClicked()
    {
        QString curPath = QFileInfo(edit->text()).absoluteDir().path();

        if (curPath.isEmpty())
            curPath = default_path;

        QString path = OpenFile(App()->GetMainWindow(), tr("Browse"),
            curPath, filter);
        if (path.isEmpty())
            return;

        edit->setText(path);
    }

public:
    EditableItemDialog(QWidget* parent, const QString& text, bool browse,
        const char* filter_ = nullptr,
        const char* default_path_ = nullptr)
        : QDialog(parent)
        , filter(QT_UTF8(filter_))
        , default_path(QT_UTF8(default_path_))
    {
        QHBoxLayout* topLayout = new QHBoxLayout();
        QVBoxLayout* mainLayout = new QVBoxLayout();

        edit = new QLineEdit();
        edit->setText(text);
        topLayout->addWidget(edit);
        topLayout->setAlignment(edit, Qt::AlignVCenter);

        if (browse) {
            QPushButton* browseButton = new QPushButton(tr("Browse"));
            browseButton->setProperty("themeID", "settingsButtons");
            topLayout->addWidget(browseButton);
            topLayout->setAlignment(browseButton, Qt::AlignVCenter);

            connect(browseButton, &QPushButton::clicked, this,
                &EditableItemDialog::BrowseClicked);
        }

        QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Cancel;

        QDialogButtonBox* buttonBox = new QDialogButtonBox(buttons);
        buttonBox->setCenterButtons(true);

        mainLayout->addLayout(topLayout);
        mainLayout->addWidget(buttonBox);

        setLayout(mainLayout);
        resize(QSize(400, 80));

        connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    }

    inline QString GetText() const { return edit->text(); }
};

void WidgetInfo::EditListAdd()
{
    enum ols_editable_list_type type = ols_property_editable_list_type(property);

    if (type == OLS_EDITABLE_LIST_TYPE_STRINGS) {
        EditListAddText();
        return;
    }

    /* Files and URLs */
    QMenu popup(view->window());

    QAction* action;

    action = new QAction(tr("Basic.PropertiesWindow.AddFiles"), this);
    connect(action, &QAction::triggered, this,
        &WidgetInfo::EditListAddFiles);
    popup.addAction(action);

    action = new QAction(tr("Basic.PropertiesWindow.AddDir"), this);
    connect(action, &QAction::triggered, this, &WidgetInfo::EditListAddDir);
    popup.addAction(action);

    if (type == OLS_EDITABLE_LIST_TYPE_FILES_AND_URLS) {
        action = new QAction(tr("Basic.PropertiesWindow.AddURL"),
            this);
        connect(action, &QAction::triggered, this,
            &WidgetInfo::EditListAddText);
        popup.addAction(action);
    }

    popup.exec(QCursor::pos());
}

void WidgetInfo::EditListAddText()
{
    QListWidget* list = reinterpret_cast<QListWidget*>(widget);
    const char* desc = ols_property_description(property);

    EditableItemDialog dialog(widget->window(), QString(), false);
    auto title = tr("Basic.PropertiesWindow.AddEditableListEntry")
                     .arg(QT_UTF8(desc));
    dialog.setWindowTitle(title);
    if (dialog.exec() == QDialog::Rejected)
        return;

    QString text = dialog.GetText();
    if (text.isEmpty())
        return;

    list->addItem(text);
    EditableListChanged();
}

void WidgetInfo::EditListAddFiles()
{
    QListWidget* list = reinterpret_cast<QListWidget*>(widget);
    const char* desc = ols_property_description(property);
    const char* filter = ols_property_editable_list_filter(property);
    const char* default_path = ols_property_editable_list_default_path(property);

    QString title = tr("Basic.PropertiesWindow.AddEditableListFiles")
                        .arg(QT_UTF8(desc));

    QStringList files = OpenFiles(App()->GetMainWindow(), title,
        QT_UTF8(default_path), QT_UTF8(filter));

    if (files.count() == 0)
        return;

    list->addItems(files);

    EditableListChanged();
}

void WidgetInfo::EditListAddDir()
{
    QListWidget* list = reinterpret_cast<QListWidget*>(widget);
    const char* desc = ols_property_description(property);
    const char* default_path = ols_property_editable_list_default_path(property);

    QString title = tr("Basic.PropertiesWindow.AddEditableListDir")
                        .arg(QT_UTF8(desc));

    QString dir = SelectDirectory(App()->GetMainWindow(), title,
        QT_UTF8(default_path));

    if (dir.isEmpty())
        return;

    list->addItem(dir);

    EditableListChanged();
}

void WidgetInfo::EditListRemove()
{
    QListWidget* list = reinterpret_cast<QListWidget*>(widget);
    QList<QListWidgetItem*> items = list->selectedItems();

    for (QListWidgetItem* item : items)
        delete item;
    EditableListChanged();
}

void WidgetInfo::EditListEdit()
{
    QListWidget* list = reinterpret_cast<QListWidget*>(widget);
    enum ols_editable_list_type type = ols_property_editable_list_type(property);
    const char* desc = ols_property_description(property);
    const char* filter = ols_property_editable_list_filter(property);
    QList<QListWidgetItem*> selectedItems = list->selectedItems();

    if (!selectedItems.count())
        return;

    QListWidgetItem* item = selectedItems[0];

    if (type == OLS_EDITABLE_LIST_TYPE_FILES) {
        QDir pathDir(item->text());
        QString path;

        if (pathDir.exists())
            path = SelectDirectory(App()->GetMainWindow(),
                tr("Browse"), item->text());
        else
            path = OpenFile(App()->GetMainWindow(), tr("Browse"),
                item->text(), QT_UTF8(filter));

        if (path.isEmpty())
            return;

        item->setText(path);
        EditableListChanged();
        return;
    }

    EditableItemDialog dialog(widget->window(), item->text(),
        type != OLS_EDITABLE_LIST_TYPE_STRINGS,
        filter);
    auto title = tr("Basic.PropertiesWindow.EditEditableListEntry")
                     .arg(QT_UTF8(desc));
    dialog.setWindowTitle(title);
    if (dialog.exec() == QDialog::Rejected)
        return;

    QString text = dialog.GetText();
    if (text.isEmpty())
        return;

    item->setText(text);
    EditableListChanged();
}

void WidgetInfo::EditListUp()
{
    QListWidget* list = reinterpret_cast<QListWidget*>(widget);
    int lastItemRow = -1;

    for (int i = 0; i < list->count(); i++) {
        QListWidgetItem* item = list->item(i);
        if (!item->isSelected())
            continue;

        int row = list->row(item);

        if ((row - 1) != lastItemRow) {
            lastItemRow = row - 1;
            list->takeItem(row);
            list->insertItem(lastItemRow, item);
            item->setSelected(true);
        } else {
            lastItemRow = row;
        }
    }

    EditableListChanged();
}

void WidgetInfo::EditListDown()
{
    QListWidget* list = reinterpret_cast<QListWidget*>(widget);
    int lastItemRow = list->count();

    for (int i = list->count() - 1; i >= 0; i--) {
        QListWidgetItem* item = list->item(i);
        if (!item->isSelected())
            continue;

        int row = list->row(item);

        if ((row + 1) != lastItemRow) {
            lastItemRow = row + 1;
            list->takeItem(row);
            list->insertItem(lastItemRow, item);
            item->setSelected(true);
        } else {
            lastItemRow = row;
        }
    }

    EditableListChanged();
}
