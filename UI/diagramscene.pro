QT += widgets
requires(qtConfig(fontcombobox))

HEADERS	    =   mainwindow.h \
		diagramitem.h \
		diagramscene.h \
		arrow.h \
		diagramtextitem.h \
		double-slider.h \
		forms/OLSBasicProperties.h \
		olsapp.h \
		plain-text-edit.h \
		properties-view.h \
		qt-wrappers.h \
		slider-ignorewheel.h \
		spinbox-ignorewheel.h \
		vertical-scroll-area.h
SOURCES	    =   mainwindow.cpp \
		diagramitem.cpp \
		double-slider.cpp \
		forms/OLSBasicPropertiescpp.cpp \
		main.cpp \
		arrow.cpp \
		diagramtextitem.cpp \
		diagramscene.cpp \
		olsapp.cpp \
		plain-text-edit.cpp \
		properties-view.cpp \
		qt-wrappers.cpp \
		slider-ignorewheel.cpp \
		spinbox-ignorewheel.cpp \
		vertical-scroll-area.cpp
RESOURCES   =	diagramscene.qrc



INCLUDEPATH += /home/akun/Open-Log-Stream-Analysis/libols


#Lib Path
LIBS += -L /home/akun/Open-Log-Stream-Analysis/build/libols -lols


# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/diagramscene
INSTALLS += target

FORMS += \
    forms/OLSBasicProperties.ui
