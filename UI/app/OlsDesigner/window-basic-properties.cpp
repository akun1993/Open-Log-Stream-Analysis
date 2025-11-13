

/******************************************************************************
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


#include "window-basic-properties.h"
#include "qt-wrappers.h"
#include "properties-view.h"
#include <QCloseEvent>
#include <QScreen>
#include <QWindow>
#include <QMessageBox>
#include <ols-data.h>
#include <ols.h>
#include <qpointer.h>
#include <util/c99defs.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif

using namespace std;

static void CreateTransitionScene(OBSSource scene, const char *text,
                  uint32_t color);


BasicProperties::BasicProperties()
{

}

BasicProperties::BasicProperties(QWidget *parent, OBSSource source_)
    : QDialog(parent),
      ui(new Ui::OLSBasicProperties),
      //main(qobject_cast<OBSBasic *>(parent)),
      acceptClicked(false),
      source(source_),
      removedSignal(obs_source_get_signal_handler(source), "remove",
            BasicProperties::SourceRemoved, this),
      renamedSignal(obs_source_get_signal_handler(source), "rename",
            BasicProperties::SourceRenamed, this),
      oldSettings(ols_data_create())
{
    int cx = (int)config_get_int(App()->GlobalConfig(), "PropertiesWindow",
                     "cx");
    int cy = (int)config_get_int(App()->GlobalConfig(), "PropertiesWindow",
                     "cy");

    enum obs_source_type type = ols_source_get_type(source);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->setupUi(this);

    if (cx > 400 && cy > 400)
        resize(cx, cy);

    /* The OBSData constructor increments the reference once */
    obs_data_release(oldSettings);

    OBSDataAutoRelease nd_settings = obs_source_get_settings(source);
    obs_data_apply(oldSettings, nd_settings);

    view = new OLSPropertiesView(
        nd_settings.Get(), source,
        (PropertiesReloadCallback)obs_source_properties,
        (PropertiesUpdateCallback) nullptr, // No special handling required for undo/redo
        (PropertiesVisualUpdateCb)obs_source_update);
    view->setMinimumHeight(150);

    ui->propertiesLayout->addWidget(view);

    if (type == OBS_SOURCE_TYPE_TRANSITION) {
        connect(view, SIGNAL(PropertiesRefreshed()), this,
            SLOT(AddPreviewButton()));
    }

    view->show();
    installEventFilter(CreateShortcutFilter());

    const char *name = obs_source_get_name(source);
    setWindowTitle(QTStr("Basic.PropertiesWindow").arg(QT_UTF8(name)));

    obs_source_inc_showing(source);

    updatePropertiesSignal.Connect(obs_source_get_signal_handler(source),
                       "update_properties",
                       BasicProperties::UpdateProperties,
                       this);

    auto addDrawCallback = [this]() {
        obs_display_add_draw_callback(ui->preview->GetDisplay(),
                          BasicProperties::DrawPreview,
                          this);
    };
    auto addTransitionDrawCallback = [this]() {
        obs_display_add_draw_callback(
            ui->preview->GetDisplay(),
            BasicProperties::DrawTransitionPreview, this);
    };
    uint32_t caps = obs_source_get_output_flags(source);
    bool drawable_type = type == OBS_SOURCE_TYPE_INPUT ||
                 type == OBS_SOURCE_TYPE_SCENE;
    bool drawable_preview = (caps & OBS_SOURCE_VIDEO) != 0;

    if (drawable_preview && drawable_type) {
        ui->preview->show();
        connect(ui->preview, &OBSQTDisplay::DisplayCreated,
            addDrawCallback);

    } else if (type == OBS_SOURCE_TYPE_TRANSITION) {
        sourceA =
            obs_source_create_private("scene", "sourceA", nullptr);
        sourceB =
            obs_source_create_private("scene", "sourceB", nullptr);

        uint32_t colorA = 0xFFB26F52;
        uint32_t colorB = 0xFF6FB252;

        CreateTransitionScene(sourceA.Get(), "A", colorA);
        CreateTransitionScene(sourceB.Get(), "B", colorB);

        /**
         * The cloned source is made from scratch, rather than using
         * obs_source_duplicate, as the stinger transition would not
         * play correctly otherwise.
         */

        OBSDataAutoRelease settings = obs_source_get_settings(source);

        sourceClone = obs_source_create_private(
            obs_source_get_id(source), "clone", settings);

        obs_source_inc_active(sourceClone);
        obs_transition_set(sourceClone, sourceA);

        auto updateCallback = [=]() {
            OBSDataAutoRelease settings =
                obs_source_get_settings(source);
            obs_source_update(sourceClone, settings);

            obs_transition_clear(sourceClone);
            obs_transition_set(sourceClone, sourceA);
            obs_transition_force_stop(sourceClone);

            direction = true;
        };

        connect(view, &OBSPropertiesView::Changed, updateCallback);

        ui->preview->show();
        connect(ui->preview, &OBSQTDisplay::DisplayCreated,
            addTransitionDrawCallback);

    } else {
        ui->preview->hide();
    }
}

BasicProperties::~BasicProperties()
{
    if (sourceClone) {
        obs_source_dec_active(sourceClone);
    }
    obs_source_dec_showing(source);
    main->SaveProject();
    main->UpdateContextBarDeferred(true);
}



static obs_source_t *CreateLabel(const char *name, size_t h)
{
    OBSDataAutoRelease settings = obs_data_create();
    OBSDataAutoRelease font = obs_data_create();

    std::string text;
    text += " ";
    text += name;
    text += " ";

#if defined(_WIN32)
    obs_data_set_string(font, "face", "Arial");
#elif defined(__APPLE__)
    obs_data_set_string(font, "face", "Helvetica");
#else
    obs_data_set_string(font, "face", "Monospace");
#endif
    obs_data_set_int(font, "flags", 1); // Bold text
    obs_data_set_int(font, "size", min(int(h), 300));

    obs_data_set_obj(settings, "font", font);
    obs_data_set_string(settings, "text", text.c_str());
    obs_data_set_bool(settings, "outline", false);

#ifdef _WIN32
    const char *text_source_id = "text_gdiplus";
#else
    const char *text_source_id = "text_ft2_source";
#endif

    obs_source_t *txtSource =
        obs_source_create_private(text_source_id, name, settings);

    return txtSource;
}


void BasicProperties::SourceRemoved(void *data, calldata_t *params)
{
    QMetaObject::invokeMethod(static_cast<BasicProperties *>(data),
                  "close");

    UNUSED_PARAMETER(params);
}

void BasicProperties::SourceRenamed(void *data, calldata_t *params)
{
    const char *name = calldata_string(params, "new_name");
    QString title = QTStr("Basic.PropertiesWindow").arg(QT_UTF8(name));

    QMetaObject::invokeMethod(static_cast<BasicProperties *>(data),
                  "setWindowTitle", Q_ARG(QString, title));
}

void BasicProperties::UpdateProperties(void *data, calldata_t *)
{
    QMetaObject::invokeMethod(static_cast<BasicProperties *>(data)->view,
                  "ReloadProperties");
}

void BasicProperties::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);

    if (val == QDialogButtonBox::AcceptRole) {

        std::string scene_name =
            obs_source_get_name(main->GetCurrentSceneSource());

        auto undo_redo = [scene_name](const std::string &data) {
            OBSDataAutoRelease settings =
                obs_data_create_from_json(data.c_str());
            OBSSourceAutoRelease source = obs_get_source_by_name(
                obs_data_get_string(settings, "undo_sname"));
            obs_source_reset_settings(source, settings);

            obs_source_update_properties(source);

            OBSSourceAutoRelease scene_source =
                obs_get_source_by_name(scene_name.c_str());

            OBSBasic::Get()->SetCurrentScene(scene_source.Get(),
                             true);
        };

        OBSDataAutoRelease new_settings = obs_data_create();
        OBSDataAutoRelease curr_settings =
            obs_source_get_settings(source);
        obs_data_apply(new_settings, curr_settings);
        obs_data_set_string(new_settings, "undo_sname",
                    obs_source_get_name(source));
        obs_data_set_string(oldSettings, "undo_sname",
                    obs_source_get_name(source));

        std::string undo_data(obs_data_get_json(oldSettings));
        std::string redo_data(obs_data_get_json(new_settings));

        if (undo_data.compare(redo_data) != 0)
            main->undo_s.add_action(
                QTStr("Undo.Properties")
                    .arg(obs_source_get_name(source)),
                undo_redo, undo_redo, undo_data, redo_data);

        acceptClicked = true;
        close();

        if (view->DeferUpdate())
            view->UpdateSettings();

    } else if (val == QDialogButtonBox::RejectRole) {
        OBSDataAutoRelease settings = obs_source_get_settings(source);
        obs_data_clear(settings);

        if (view->DeferUpdate())
            obs_data_apply(settings, oldSettings);
        else
            obs_source_update(source, oldSettings);

        close();

    } else if (val == QDialogButtonBox::ResetRole) {
        OBSDataAutoRelease settings = obs_source_get_settings(source);
        obs_data_clear(settings);

        if (!view->DeferUpdate())
            obs_source_update(source, nullptr);

        view->ReloadProperties();
    }
}



void BasicProperties::Cleanup()
{
    config_set_int(App()->GlobalConfig(), "PropertiesWindow", "cx",
               width());
    config_set_int(App()->GlobalConfig(), "PropertiesWindow", "cy",
               height());

    obs_display_remove_draw_callback(ui->preview->GetDisplay(),
                     BasicProperties::DrawPreview, this);
    obs_display_remove_draw_callback(
        ui->preview->GetDisplay(),
        BasicProperties::DrawTransitionPreview, this);
}

void BasicProperties::reject()
{
    if (!acceptClicked && (CheckSettings() != 0)) {
        if (!ConfirmQuit()) {
            return;
        }
    }

    Cleanup();
    done(0);
}

void BasicProperties::closeEvent(QCloseEvent *event)
{
    if (!acceptClicked && (CheckSettings() != 0)) {
        if (!ConfirmQuit()) {
            event->ignore();
            return;
        }
    }

    QDialog::closeEvent(event);
    if (!event->isAccepted())
        return;

    Cleanup();
}

void BasicProperties::Init()
{
    show();
}

int BasicProperties::CheckSettings()
{
    OBSDataAutoRelease currentSettings = obs_source_get_settings(source);
    const char *oldSettingsJson = obs_data_get_json(oldSettings);
    const char *currentSettingsJson = obs_data_get_json(currentSettings);

    return strcmp(currentSettingsJson, oldSettingsJson);
}

bool BasicProperties::ConfirmQuit()
{
    QMessageBox::StandardButton button;

    button = OLSMessageBox::question(
        this, QTStr("Basic.PropertiesWindow.ConfirmTitle"),
        QTStr("Basic.PropertiesWindow.Confirm"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    switch (button) {
    case QMessageBox::Save:
        acceptClicked = true;
        if (view->DeferUpdate())
            view->UpdateSettings();
        // Do nothing because the settings are already updated
        break;
    case QMessageBox::Discard:
        ols_source_update(source, oldSettings);
        break;
    case QMessageBox::Cancel:
        return false;
        break;
    default:
        /* If somehow the dialog fails to show, just default to
         * saving the settings. */
        break;
    }
    return true;
}

