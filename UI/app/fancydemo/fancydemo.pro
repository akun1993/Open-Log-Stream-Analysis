QTC_LIB_DEPENDS += qcanpool
include(../../qtproject.pri)

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TEMPLATE = app
TARGET = fancydemo
DESTDIR = $$IDE_APP_PATH

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include($$QTCANPOOL_DIR/lib/rpath.pri)

SOURCES += \
    AdvancedToolBox.cpp \
    DesignWid.cpp \
    FlowChartScene.cpp \
    FlowChartView.cpp \
    FlowEdge.cpp \
    FlowNode.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    AdvancedToolBox.h \
    DesignWid.h \
    FlowChartScene.h \
    FlowChartView.h \
    FlowEdge.h \
    FlowNode.h \
    mainwindow.h

RESOURCES += \
    fancydemo.qrc

LIBS+= qcanpool

RC_FILE = fancydemo.rc
