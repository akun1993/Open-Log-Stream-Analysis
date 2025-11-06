

#include "FlowEdge.h"
#include "FlowChartScene.h"
#include "FlowChartView.h"
#include "AdvancedToolBox.h"
#include "PluginButton.h"
#include <QScrollArea>
#include "DesignWid.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets>

const int InsertTextButton = 10;

//! [0]
DesignWid::DesignWid()
{
    createActions();
    createToolBox();

    scene = new FlowChartScene(itemMenu, this);
    scene->setSceneRect(QRectF(0, 0, 3200, 1800));

    connect(scene, &FlowChartScene::itemInserted,
            this, &DesignWid::itemInserted);

    connect(scene, &FlowChartScene::itemSelected,
            this, &DesignWid::itemSelected);

    createToolbars();

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(toolBoxScroll);


    QWidget *editWid = new QWidget;

    QVBoxLayout *vlayout = new QVBoxLayout;

    view = new FlowChartView(scene);
    vlayout->addWidget(toolWid);
    vlayout->addWidget(view);

    editWid->setLayout(vlayout);

    hlayout->addWidget(editWid);

    //QWidget *widget = new QWidget;
    setLayout(hlayout);

    setWindowTitle(tr("Diagramscene"));

}
//! [0]

//! [1]
void DesignWid::backgroundButtonGroupClicked(QAbstractButton *button)
{
    const QList<QAbstractButton *> buttons = processGroup->buttons();
    for (QAbstractButton *myButton : buttons) {
        if (myButton != button)
            button->setChecked(false);
    }

//   QString text = button->text();
//   if (text == tr("Blue Grid"))
//        scene->setBackgroundBrush(QPixmap(":/images/background1.png"));
//    else if (text == tr("White Grid"))
//        scene->setBackgroundBrush(QPixmap(":/images/background2.png"));
//    else if (text == tr("Gray Grid"))
//        scene->setBackgroundBrush(QPixmap(":/images/background3.png"));
//    else
//        scene->setBackgroundBrush(QPixmap(":/images/background4.png"));

    scene->update();
    view->update();
}
//! [1]

//! [2]
void DesignWid::inputGroupClicked(QAbstractButton *button)
{
    const QList<QAbstractButton *> buttons = inputGroup->buttons();
    for (QAbstractButton *myButton : buttons) {
        if (myButton != button)
            button->setChecked(false);
    }
    PluginButton *pluginBtn =qobject_cast<PluginButton *>(button);
    const int id = inputGroup->id(button);

    {
        scene->setItemInfo(FlowNode::FlowNodeType(id),pluginBtn->name());
        scene->setMode(FlowChartScene::InsertItem);
    }
}


void DesignWid::processGroupClicked(QAbstractButton *button)
{
    const QList<QAbstractButton *> buttons = processGroup->buttons();
    for (QAbstractButton *myButton : buttons) {
        if (myButton != button)
            button->setChecked(false);
    }

    const int id = processGroup->id(button);


    PluginButton *pluginBtn =qobject_cast<PluginButton *>(button);

    {
        scene->setItemInfo(FlowNode::FlowNodeType(id),pluginBtn->name());
        scene->setMode(FlowChartScene::InsertItem);
    }
}


void DesignWid::outputGroupClicked(QAbstractButton *button)
{
    const QList<QAbstractButton *> buttons = outputGroup->buttons();
    for (QAbstractButton *myButton : buttons) {
        if (myButton != button)
            button->setChecked(false);
    }

    PluginButton *pluginBtn =qobject_cast<PluginButton *>(button);

    const int id = outputGroup->id(button);

    {
        scene->setItemInfo(FlowNode::FlowNodeType(id),pluginBtn->name());
        scene->setMode(FlowChartScene::InsertItem);
    }
}

//! [2]

//! [3]
void DesignWid::deleteItem()
{
    QList<QGraphicsItem *> selectedItems = scene->selectedItems();
    for (QGraphicsItem *item : qAsConst(selectedItems)) {
        if (item->type() == FlowEdge::Type) {
            scene->removeItem(item);
            FlowEdge *arrow = qgraphicsitem_cast<FlowEdge *>(item);
            arrow->sourceNode()->removeArrow(arrow);
            arrow->targetNode()->removeArrow(arrow);
            delete item;
        }
    }

    selectedItems = scene->selectedItems();
    for (QGraphicsItem *item : qAsConst(selectedItems)) {
         if (item->type() == FlowNode::Type)
             qgraphicsitem_cast<FlowNode *>(item)->removeArrows();
         scene->removeItem(item);
         delete item;
     }
}
//! [3]

//! [4]
void DesignWid::pointerGroupClicked()
{
    scene->setMode(FlowChartScene::Mode(pointerTypeGroup->checkedId()));
}
//! [4]

//! [5]
void DesignWid::bringToFront()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    const QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    for (const QGraphicsItem *item : overlapItems) {
        if (item->zValue() >= zValue && item->type() == FlowNode::Type)
            zValue = item->zValue() + 0.1;
    }
    selectedItem->setZValue(zValue);
}
//! [5]

//! [6]
void DesignWid::sendToBack()
{
    if (scene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    const QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    for (const QGraphicsItem *item : overlapItems) {
        if (item->zValue() <= zValue && item->type() == FlowNode::Type)
            zValue = item->zValue() - 0.1;
    }
    selectedItem->setZValue(zValue);
}
//! [6]

//! [7]
void DesignWid::itemInserted(FlowNode *item)
{
    pointerTypeGroup->button(int(FlowChartScene::MoveItem))->setChecked(true);
    scene->setMode(FlowChartScene::Mode(pointerTypeGroup->checkedId()));


    inputGroup->button(int(item->flowNodeType()))->setChecked(false);
}



void DesignWid::currentFontChanged(const QFont &)
{
    handleFontChange();
}


void DesignWid::fontSizeChanged(const QString &)
{
    handleFontChange();
}


void DesignWid::sceneScaleChanged(const QString &scale)
{
    double newScale = scale.left(scale.indexOf(tr("%"))).toDouble() / 100.0;
    QTransform oldMatrix = view->transform();
    view->resetTransform();
    view->translate(oldMatrix.dx(), oldMatrix.dy());
    view->scale(newScale, newScale);
}


void DesignWid::textColorChanged()
{
    textAction = qobject_cast<QAction *>(sender());
    fontColorToolButton->setIcon(createColorToolButtonIcon(
                                     ":/images/textpointer.png",
                                     qvariant_cast<QColor>(textAction->data())));
    textButtonTriggered();
}

void DesignWid::itemColorChanged()
{
    fillAction = qobject_cast<QAction *>(sender());
    fillColorToolButton->setIcon(createColorToolButtonIcon(
                                     ":/images/floodfill.png",
                                     qvariant_cast<QColor>(fillAction->data())));
    fillButtonTriggered();
}


void DesignWid::lineColorChanged()
{
    lineAction = qobject_cast<QAction *>(sender());
    lineColorToolButton->setIcon(createColorToolButtonIcon(
                                     ":/images/linecolor.png",
                                     qvariant_cast<QColor>(lineAction->data())));
    lineButtonTriggered();
}


void DesignWid::textButtonTriggered()
{
   // scene->setTextColor(qvariant_cast<QColor>(textAction->data()));
}


void DesignWid::fillButtonTriggered()
{
    //scene->setItemColor(qvariant_cast<QColor>(fillAction->data()));
}

void DesignWid::lineButtonTriggered()
{
   // scene->setLineColor(qvariant_cast<QColor>(lineAction->data()));
}


void DesignWid::handleFontChange()
{
    QFont font = fontCombo->currentFont();
    font.setPointSize(fontSizeCombo->currentText().toInt());
    font.setWeight(boldAction->isChecked() ? QFont::Bold : QFont::Normal);
    font.setItalic(italicAction->isChecked());
    font.setUnderline(underlineAction->isChecked());

   // scene->setFont(font);
}


void DesignWid::itemSelected(QGraphicsItem *item)
{
//    DiagramTextItem *textItem =
//    qgraphicsitem_cast<DiagramTextItem *>(item);

//    QFont font = textItem->font();
//    fontCombo->setCurrentFont(font);
//    fontSizeCombo->setEditText(QString().setNum(font.pointSize()));
//    boldAction->setChecked(font.weight() == QFont::Bold);
//    italicAction->setChecked(font.italic());
//    underlineAction->setChecked(font.underline());
}


void DesignWid::about()
{
    QMessageBox::about(this, tr("About Diagram Scene"),
                       tr("The <b>Diagram Scene</b> example shows "
                          "use of the graphics framework."));
}


void DesignWid::createToolBox()
{
    inputGroup = new QButtonGroup(this);
    inputGroup->setExclusive(false);
    connect(inputGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &DesignWid::inputGroupClicked);

    QGridLayout *inputLayout = new QGridLayout;

    QIcon icon;

    inputLayout->addWidget(createCellWidget(tr("File+"), inputGroup,FlowNode::INPUT,icon), 0, 0);
    inputLayout->addWidget(createCellWidget(tr("Serial"), inputGroup,FlowNode::INPUT,icon),0, 1);
    inputLayout->addWidget(createCellWidget(tr("Net"), inputGroup,FlowNode::INPUT,icon), 1, 0);


    inputLayout->setRowStretch(3, 10);
    inputLayout->setColumnStretch(2, 10);


    QWidget *inputWidget = new QWidget;
    inputWidget->setLayout(inputLayout);

    //start process group
    processGroup = new QButtonGroup(this);
    connect(processGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &DesignWid::processGroupClicked);

    QGridLayout *processLayout = new QGridLayout;

    processLayout->addWidget(createCellWidget(tr("Blue Grid"), processGroup,FlowNode::PROCESS,icon), 0, 0);

    processLayout->addWidget(createCellWidget(tr("White Grid"),processGroup,FlowNode::PROCESS,icon), 0, 1);

    processLayout->addWidget(createCellWidget(tr("Gray Grid"), processGroup,FlowNode::PROCESS,icon), 1, 0);

    processLayout->setRowStretch(2, 10);
    processLayout->setColumnStretch(2, 10);

    QWidget *processWidget = new QWidget;
    processWidget->setLayout(processLayout);


    //start output button group
    outputGroup = new QButtonGroup(this);
    connect(outputGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &DesignWid::outputGroupClicked);

    QGridLayout *outputLayout = new QGridLayout;

    outputLayout->addWidget(createCellWidget(tr("Html ouput"), outputGroup,FlowNode::OUTPUT,icon), 0, 0);

    outputLayout->addWidget(createCellWidget(tr("Json output"), outputGroup,FlowNode::OUTPUT,icon), 0, 1);

    outputLayout->addWidget(createCellWidget(tr("Xml output"), outputGroup,FlowNode::OUTPUT,icon), 1, 0);

    outputLayout->setRowStretch(2, 10);
    outputLayout->setColumnStretch(2, 10);

    QWidget *outputWidget = new QWidget;
    outputWidget->setLayout(outputLayout);

    toolBoxScroll = new QScrollArea(this);
    toolBoxScroll->setWidgetResizable(true);

    toolBox = new AdvancedToolBox(this);

    toolBoxScroll->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Ignored));
    toolBoxScroll->setMinimumWidth(inputWidget->sizeHint().width());


    QIcon iconfolder(":/images/folder.svg");
    iconfolder.addFile(":/images/reopen-folder.svg", QSize(), QIcon::Normal, QIcon::On);

    toolBox->addWidget(inputWidget , "Input",iconfolder);

    toolBox->addWidget(processWidget, "Process", QIcon(":/images/smile.png"));

    toolBox->addWidget(outputWidget, "Output", QIcon(":/images/user.png"));

    toolBoxScroll->setWidget(toolBox);

}


void DesignWid::createActions()
{
    toFrontAction = new QAction(QIcon(":/images/bringtofront.png"),
                                tr("Bring to &Front"), this);
    toFrontAction->setShortcut(tr("Ctrl+F"));
    toFrontAction->setStatusTip(tr("Bring item to front"));
    connect(toFrontAction, &QAction::triggered, this, &DesignWid::bringToFront);


    sendBackAction = new QAction(QIcon(":/images/sendtoback.png"), tr("Send to &Back"), this);
    sendBackAction->setShortcut(tr("Ctrl+T"));
    sendBackAction->setStatusTip(tr("Send item to back"));
    connect(sendBackAction, &QAction::triggered, this, &DesignWid::sendToBack);

    deleteAction = new QAction(QIcon(":/images/delete.png"), tr("&Delete"), this);
    deleteAction->setShortcut(tr("Delete"));
    deleteAction->setStatusTip(tr("Delete item from diagram"));
    connect(deleteAction, &QAction::triggered, this, &DesignWid::deleteItem);

    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcuts(QKeySequence::Quit);
    exitAction->setStatusTip(tr("Quit Scenediagram example"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    boldAction = new QAction(tr("Bold"), this);
    boldAction->setCheckable(true);
    QPixmap pixmap(":/images/bold.png");
    boldAction->setIcon(QIcon(pixmap));
    boldAction->setShortcut(tr("Ctrl+B"));
    connect(boldAction, &QAction::triggered, this, &DesignWid::handleFontChange);

    italicAction = new QAction(QIcon(":/images/italic.png"), tr("Italic"), this);
    italicAction->setCheckable(true);
    italicAction->setShortcut(tr("Ctrl+I"));
    connect(italicAction, &QAction::triggered, this, &DesignWid::handleFontChange);

    underlineAction = new QAction(QIcon(":/images/underline.png"), tr("Underline"), this);
    underlineAction->setCheckable(true);
    underlineAction->setShortcut(tr("Ctrl+U"));
    connect(underlineAction, &QAction::triggered, this, &DesignWid::handleFontChange);

}



void DesignWid::createToolbars()
{

    toolWid  = new QWidget();
    QHBoxLayout *toolBarLayout = new QHBoxLayout( );

    editToolBar = new QToolBar(tr("Edit"),this);
    editToolBar->addAction(deleteAction);
    editToolBar->addAction(toFrontAction);
    editToolBar->addAction(sendBackAction);

    toolBarLayout->addWidget(editToolBar);

    QSpacerItem* spacer = new QSpacerItem(10, 160, QSizePolicy::Fixed, QSizePolicy::Fixed);
    toolBarLayout->addSpacerItem(spacer);


    fontCombo = new QFontComboBox();
    connect(fontCombo, &QFontComboBox::currentFontChanged,
            this, &DesignWid::currentFontChanged);

    fontSizeCombo = new QComboBox;
    fontSizeCombo->setEditable(true);
    for (int i = 8; i < 30; i = i + 2)
        fontSizeCombo->addItem(QString().setNum(i));
    QIntValidator *validator = new QIntValidator(2, 64, this);
    fontSizeCombo->setValidator(validator);
    connect(fontSizeCombo, &QComboBox::currentTextChanged,
            this, &DesignWid::fontSizeChanged);

    fontColorToolButton = new QToolButton;
    fontColorToolButton->setPopupMode(QToolButton::MenuButtonPopup);
    fontColorToolButton->setMenu(createColorMenu(SLOT(textColorChanged()), Qt::black));
    textAction = fontColorToolButton->menu()->defaultAction();
    fontColorToolButton->setIcon(createColorToolButtonIcon(":/images/textpointer.png", Qt::black));
    fontColorToolButton->setAutoFillBackground(true);
    connect(fontColorToolButton, &QAbstractButton::clicked,
            this, &DesignWid::textButtonTriggered);


    fillColorToolButton = new QToolButton;
    fillColorToolButton->setPopupMode(QToolButton::MenuButtonPopup);
    fillColorToolButton->setMenu(createColorMenu(SLOT(itemColorChanged()), Qt::white));
    fillAction = fillColorToolButton->menu()->defaultAction();
    fillColorToolButton->setIcon(createColorToolButtonIcon(
                                     ":/images/floodfill.png", Qt::white));
    connect(fillColorToolButton, &QAbstractButton::clicked,
            this, &DesignWid::fillButtonTriggered);

    lineColorToolButton = new QToolButton;
    lineColorToolButton->setPopupMode(QToolButton::MenuButtonPopup);
    lineColorToolButton->setMenu(createColorMenu(SLOT(lineColorChanged()), Qt::black));
    lineAction = lineColorToolButton->menu()->defaultAction();
    lineColorToolButton->setIcon(createColorToolButtonIcon(
                                     ":/images/linecolor.png", Qt::black));
    connect(lineColorToolButton, &QAbstractButton::clicked,
            this, &DesignWid::lineButtonTriggered);

    textToolBar = new QToolBar(tr("Font"),this);
    textToolBar->addWidget(fontCombo);
    textToolBar->addWidget(fontSizeCombo);
    textToolBar->addAction(boldAction);
    textToolBar->addAction(italicAction);
    textToolBar->addAction(underlineAction);

    toolBarLayout->addWidget(textToolBar);
    spacer = new QSpacerItem(10, 160, QSizePolicy::Fixed, QSizePolicy::Fixed);
    toolBarLayout->addSpacerItem(spacer);

    colorToolBar = new QToolBar(tr("Color"),this);
    colorToolBar->addWidget(fontColorToolButton);
    colorToolBar->addWidget(fillColorToolButton);
    colorToolBar->addWidget(lineColorToolButton);

    toolBarLayout->addWidget(colorToolBar);

    QToolButton *pointerButton = new QToolButton;
    pointerButton->setCheckable(true);
    pointerButton->setChecked(true);
    pointerButton->setIcon(QIcon(":/images/pointer.png"));
    QToolButton *linePointerButton = new QToolButton;
    linePointerButton->setCheckable(true);
    linePointerButton->setIcon(QIcon(":/images/linepointer.png"));

    pointerTypeGroup = new QButtonGroup(this);
    pointerTypeGroup->addButton(pointerButton, int(FlowChartScene::MoveItem));
    pointerTypeGroup->addButton(linePointerButton, int(FlowChartScene::InsertLine));
    connect(pointerTypeGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &DesignWid::pointerGroupClicked);

    sceneScaleCombo = new QComboBox;
    QStringList scales;
    scales << tr("50%") << tr("75%") << tr("100%") << tr("125%") << tr("150%");
    sceneScaleCombo->addItems(scales);
    sceneScaleCombo->setCurrentIndex(2);
    connect(sceneScaleCombo, &QComboBox::currentTextChanged,
            this, &DesignWid::sceneScaleChanged);

    pointerToolbar = new QToolBar(tr("Pointer type"),this); //addToolBar(tr("Pointer type"));
    pointerToolbar->addWidget(pointerButton);
    pointerToolbar->addWidget(linePointerButton);
    pointerToolbar->addWidget(sceneScaleCombo);

    toolBarLayout->addWidget(pointerToolbar);
    spacer = new QSpacerItem(0, 160, QSizePolicy::Expanding, QSizePolicy::Fixed);
    toolBarLayout->addSpacerItem(spacer);

    toolWid->setLayout(toolBarLayout);

    toolWid->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum));

    int max_height = qMax(textToolBar->sizeHint().height(),qMax(editToolBar->sizeHint().height(),pointerToolbar->sizeHint().height()));

    toolWid->setMaximumHeight(max_height + 20);

}



QWidget *DesignWid::createBackgroundCellWidget(const QString &text, const QString &image)
{
    QToolButton *button = new QToolButton;
    button->setText(text);
    button->setIcon(QIcon(image));
    button->setIconSize(QSize(50, 50));
    button->setCheckable(true);
    processGroup->addButton(button);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(button, 0, 0, Qt::AlignHCenter);
    layout->addWidget(new QLabel(text), 1, 0, Qt::AlignCenter);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    return widget;
}



QWidget *DesignWid::createCellWidget(const QString &text,  QButtonGroup * group,FlowNode::FlowNodeType type,const QIcon &icon,const QString &name)
{

    PluginButton *button = new PluginButton;
    button->setIcon(icon);
    button->setIconSize(QSize(50, 50));
    button->setCheckable(true);
    button->setName(name);

    group->addButton(button, int(type));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(button, 0, 0, Qt::AlignHCenter);
    layout->addWidget(new QLabel(text), 1, 0, Qt::AlignCenter);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    return widget;
}


QMenu *DesignWid::createColorMenu(const char *slot, QColor defaultColor)
{
    QList<QColor> colors;
    colors << Qt::black << Qt::white << Qt::red << Qt::blue << Qt::yellow;
    QStringList names;
    names << tr("black") << tr("white") << tr("red") << tr("blue")
          << tr("yellow");

    QMenu *colorMenu = new QMenu(this);
    for (int i = 0; i < colors.count(); ++i) {
        QAction *action = new QAction(names.at(i), this);
        action->setData(colors.at(i));

        action->setIcon(createColorIcon(colors.at(i)));

        connect(action, SIGNAL(triggered()), this, slot);
        colorMenu->addAction(action);
        if (colors.at(i) == defaultColor)
            colorMenu->setDefaultAction(action);
    }
    return colorMenu;
}


QIcon DesignWid::createColorToolButtonIcon(const QString &imageFile, QColor color)
{
    QPixmap pixmap(50, 80);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    QPixmap image(imageFile);
    // Draw icon centred horizontally on button.
    QRect target(4, 0, 42, 43);
    QRect source(0, 0, 42, 43);
    painter.fillRect(QRect(0, 60, 50, 80), color);
    painter.drawPixmap(target, image, source);

    return QIcon(pixmap);
}


QIcon DesignWid::createColorIcon(QColor color)
{
    QPixmap pixmap(20, 20);
    QPainter painter(&pixmap);
    painter.setPen(Qt::NoPen);
    painter.fillRect(QRect(0, 0, 20, 20), color);

    return QIcon(pixmap);
}

