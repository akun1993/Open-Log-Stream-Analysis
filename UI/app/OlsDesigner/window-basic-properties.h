#ifndef BASICPROPERTIES_H
#define BASICPROPERTIES_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QSplitter>
#include <memory>

class BasicProperties : public QDialog
{
    Q_OBJECT
public:
    BasicProperties();


    std::unique_ptr<Ui::OLSBasicProperties> ui;
    bool acceptClicked;

    OBSSource source;
    OBSSignal removedSignal;
    OBSSignal renamedSignal;

    OBSSignal updatePropertiesSignal;
    OBSData oldSettings;

    OLSPropertiesView *view;

    QDialogButtonBox *buttonBox;

    QSplitter *windowSplitter;

    OBSSourceAutoRelease sourceA;
    OBSSourceAutoRelease sourceB;
    OBSSourceAutoRelease sourceClone;
    bool direction = true;

    static void SourceRemoved(void *data, calldata_t *params);
    static void SourceRenamed(void *data, calldata_t *params);
    static void UpdateProperties(void *data, calldata_t *params);

    void UpdateCallback(void *obj, obs_data_t *settings);
    bool ConfirmQuit();
    int CheckSettings();
    void Cleanup();

private Q_SLOTS:
    void on_buttonBox_clicked(QAbstractButton *button);

public:
    BasicProperties(QWidget *parent, OBSSource source_);
    ~BasicProperties();

    void Init();

protected:
    virtual void closeEvent(QCloseEvent *event) override;
    virtual void reject() override;

};

#endif // BASICPROPERTIES_H
