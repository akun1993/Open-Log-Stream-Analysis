
#ifndef DESIGN_WID_H
#define DESIGN_WID_H


#include "FlowNode.h"
#include <QWidget>


class AdvancedToolBox;
class FlowChartScene;
class FlowChartView;

QT_BEGIN_NAMESPACE
class QAction;
class QToolBox;
class QSpinBox;
class QComboBox;
class QFontComboBox;
class QButtonGroup;
class QLineEdit;
class QGraphicsTextItem;
class QFont;
class QToolButton;
class QAbstractButton;
class QGraphicsView;
class QScrollArea;
class QToolBar;
class QHBoxLayout;
QT_END_NAMESPACE

//! [0]
class DesignWid : public QWidget
{
    Q_OBJECT

public:
   DesignWid();

private Q_SLOTS:
    void backgroundButtonGroupClicked(QAbstractButton *button);

    void inputGroupClicked(QAbstractButton *button);
    void processGroupClicked(QAbstractButton *button);
    void outputGroupClicked(QAbstractButton *button);

    void deleteItem();
    void pointerGroupClicked();
    void bringToFront();
    void sendToBack();
    void itemInserted(FlowNode *item);
    void textInserted(QGraphicsTextItem *item);
    void currentFontChanged(const QFont &font);
    void fontSizeChanged(const QString &size);
    void sceneScaleChanged(const QString &scale);
    void textColorChanged();
    void itemColorChanged();
    void lineColorChanged();
    void textButtonTriggered();
    void fillButtonTriggered();
    void lineButtonTriggered();
    void handleFontChange();
    void itemSelected(QGraphicsItem *item);
    void about();

private:
    void createToolBox();
    void createActions();
    void createToolbars();

    QWidget *createBackgroundCellWidget(const QString &text,
                                        const QString &image);

    QWidget *createCellWidget(const QString &text,  QButtonGroup * group,FlowNode::FlowNodeType type,const QIcon &icon,const QString &name = "");

    QMenu *createColorMenu(const char *slot, QColor defaultColor);
    QIcon createColorToolButtonIcon(const QString &image, QColor color);
    QIcon createColorIcon(QColor color);

    FlowChartScene *scene;
    FlowChartView *view;

    QAction *exitAction;
    QAction *addAction;
    QAction *deleteAction;

    QAction *toFrontAction;
    QAction *sendBackAction;
    QAction *aboutAction;

    QMenu *fileMenu;
    QMenu *itemMenu;
    QMenu *aboutMenu;

    QWidget  *toolWid;
    QToolBar *textToolBar;
    QToolBar *editToolBar;
    QToolBar *colorToolBar;
    QToolBar *pointerToolbar;

    QComboBox *sceneScaleCombo;
    QComboBox *itemColorCombo;
    QComboBox *textColorCombo;
    QComboBox *fontSizeCombo;
    QFontComboBox *fontCombo;

    AdvancedToolBox *toolBox;
    QScrollArea *toolBoxScroll;

    //Three plugic type
    QButtonGroup *inputGroup;
    QButtonGroup *outputGroup;
    QButtonGroup *processGroup;

    QButtonGroup *pointerTypeGroup;

    QToolButton *fontColorToolButton;
    QToolButton *fillColorToolButton;
    QToolButton *lineColorToolButton;
    QAction *boldAction;
    QAction *underlineAction;
    QAction *italicAction;
    QAction *textAction;
    QAction *fillAction;
    QAction *lineAction;
};
//! [0]

#endif // DESIGN_WID_H
