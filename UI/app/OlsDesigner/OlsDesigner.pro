QTC_LIB_DEPENDS += qcanpool
include(../../qtproject.pri)

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TEMPLATE = app
TARGET = OlsDesigner
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
    OlsApp.cpp \
    OlsMainwindow.cpp \
    PluginButton.cpp \
    double-slider.cpp \
    main.cpp \
    plain-text-edit.cpp \
    properties-view.cpp \
    qt-wrappers.cpp \
    slider-ignorewheel.cpp \
    spinbox-ignorewheel.cpp \
    vertical-scroll-area.cpp \
    window-basic-properties.cpp

HEADERS += \
    AdvancedToolBox.h \
    OlsApp.h \
    OlsMainwindow.h \
    DesignWid.h \
    FlowChartScene.h \
    FlowChartView.h \
    FlowEdge.h \
    FlowNode.h \
    PluginButton.h \
    PluginInfo.h \
    double-slider.h \
    plain-text-edit.h \
    properties-view.h \
    qt-wrappers.h \
    slider-ignorewheel.h \
    spinbox-ignorewheel.h \
    vertical-scroll-area.h \
    window-basic-properties.h

RESOURCES += \
    OlsDesigner.qrc

LIBS+= ../../../build/libols/libols.so

INCLUDEPATH += ../../../libols \
            ../../../deps/uthash-header

win32 {
  INCLUDEPATH += ../../../deps/w32-pthreads
}

RC_FILE = OlsDesigner.rc
