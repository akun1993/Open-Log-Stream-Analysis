

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



BasicProperties::BasicProperties()
{

}

BasicProperties::BasicProperties(QWidget *parent, OLSSource source_)
    : QDialog(parent),
      ui(new Ui::OLSBasicProperties),
      //main(qobject_cast<OBSBasic *>(parent)),
      acceptClicked(false),
      source(source_),
      removedSignal(ols_source_get_signal_handler(source), "remove",
            BasicProperties::SourceRemoved, this),
      renamedSignal(ols_source_get_signal_handler(source), "rename",
            BasicProperties::SourceRenamed, this),
      oldSettings(ols_data_create())
{
//    int cx = (int)config_get_int(App()->GlobalConfig(), "PropertiesWindow",
//                     "cx");
//    int cy = (int)config_get_int(App()->GlobalConfig(), "PropertiesWindow",
//                     "cy");

    enum ols_source_type type = ols_source_get_type(source);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->setupUi(this);

//    if (cx > 400 && cy > 400)
//        resize(cx, cy);

    /* The OLSData constructor increments the reference once */
    ols_data_release(oldSettings);

    OLSDataAutoRelease nd_settings = ols_source_get_settings(source);
    ols_data_apply(oldSettings, nd_settings);

    view = new OLSPropertiesView(
        nd_settings.Get(), source,
        (PropertiesReloadCallback)ols_source_properties,
        (PropertiesUpdateCallback) nullptr, // No special handling required for undo/redo
        (PropertiesVisualUpdateCb)ols_source_update);
    view->setMinimumHeight(150);

    ui->propertiesLayout->addWidget(view);


    view->show();
   // installEventFilter(CreateShortcutFilter());

    const char *name = ols_source_get_name(source);
    setWindowTitle(QTStr("Basic.PropertiesWindow").arg(QT_UTF8(name)));

  //  ols_source_inc_showing(source);

    updatePropertiesSignal.Connect(ols_source_get_signal_handler(source),
                       "update_properties",
                       BasicProperties::UpdateProperties,
                       this);


}

BasicProperties::~BasicProperties()
{

   // ols_source_dec_showing(source);
  //  main->SaveProject();
   // main->UpdateContextBarDeferred(true);
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

//        std::string scene_name =
//            ols_source_get_name(main->GetCurrentSceneSource());


//        auto undo_redo = [scene_name](const std::string &data) {
//            OLSDataAutoRelease settings =
//                ols_data_create_from_json(data.c_str());
//            OLSSourceAutoRelease source = ols_get_source_by_name(
//                ols_data_get_string(settings, "undo_sname"));

//            ols_source_reset_settings(source, settings);

//            ols_source_update_properties(source);

//            OLSSourceAutoRelease scene_source =
//                ols_get_source_by_name(scene_name.c_str());

//            OBSBasic::Get()->SetCurrentScene(scene_source.Get(),
//                             true);
//        };

//        OBSDataAutoRelease new_settings = ols_data_create();
//        OBSDataAutoRelease curr_settings =
//            ols_source_get_settings(source);
//        ols_data_apply(new_settings, curr_settings);
//        ols_data_set_string(new_settings, "undo_sname",
//                    ols_source_get_name(source));
//        ols_data_set_string(oldSettings, "undo_sname",
//                    ols_source_get_name(source));

//        std::string undo_data(ols_data_get_json(oldSettings));
//        std::string redo_data(ols_data_get_json(new_settings));

//        if (undo_data.compare(redo_data) != 0)
//            main->undo_s.add_action(
//                QTStr("Undo.Properties")
//                    .arg(ols_source_get_name(source)),
//                undo_redo, undo_redo, undo_data, redo_data);

        acceptClicked = true;
        close();

        if (view->DeferUpdate())
            view->UpdateSettings();

    } else if (val == QDialogButtonBox::RejectRole) {
        OLSDataAutoRelease settings = ols_source_get_settings(source);
        ols_data_clear(settings);

//        if (view->DeferUpdate())
//            ols_data_apply(settings, oldSettings);
//        else
//            ols_source_update(source, oldSettings);

        close();

    } else if (val == QDialogButtonBox::ResetRole) {
        OLSDataAutoRelease settings = ols_source_get_settings(source);
        ols_data_clear(settings);

        if (!view->DeferUpdate())
            ols_source_update(source, nullptr);

        view->ReloadProperties();
    }
}



void BasicProperties::Cleanup()
{
//    config_set_int(App()->GlobalConfig(), "PropertiesWindow", "cx",
//               width());
//    config_set_int(App()->GlobalConfig(), "PropertiesWindow", "cy",
//               height());

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
    OLSDataAutoRelease currentSettings = ols_source_get_settings(source);
    const char *oldSettingsJson = ols_data_get_json(oldSettings);
    const char *currentSettingsJson = ols_data_get_json(currentSettings);

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

