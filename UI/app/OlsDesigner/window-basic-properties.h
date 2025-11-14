#ifndef BASICPROPERTIES_H
#define BASICPROPERTIES_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QSplitter>
#include <memory>
#include "ols.hpp"

#include "ui_OLSBasicProperties.h"

class OLSPropertiesView;

class BasicProperties : public QDialog
{
    Q_OBJECT
public:
    BasicProperties();


    std::unique_ptr<Ui::OLSBasicProperties> ui;
    bool acceptClicked;

    OLSSource source;
    OLSSignal removedSignal;
    OLSSignal renamedSignal;

    OLSSignal updatePropertiesSignal;
    OLSData oldSettings;

    OLSPropertiesView *view;

    QDialogButtonBox *buttonBox;

    QSplitter *windowSplitter;

    OLSSourceAutoRelease sourceA;
    OLSSourceAutoRelease sourceB;
    OLSSourceAutoRelease sourceClone;

    bool direction = true;

    static void SourceRemoved(void *data, calldata_t *params);
    static void SourceRenamed(void *data, calldata_t *params);
    static void UpdateProperties(void *data, calldata_t *params);

    void UpdateCallback(void *obj, ols_data_t *settings);
    bool ConfirmQuit();
    int CheckSettings();
    void Cleanup();

private Q_SLOTS:
    void on_buttonBox_clicked(QAbstractButton *button);

public:
    BasicProperties(QWidget *parent, OLSSource source_);
    ~BasicProperties();

    void Init();

protected:
    virtual void closeEvent(QCloseEvent *event) override;
    virtual void reject() override;

};

#endif // BASICPROPERTIES_H
