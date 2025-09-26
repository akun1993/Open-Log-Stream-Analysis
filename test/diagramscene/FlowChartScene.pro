QT += widgets
requires(qtConfig(fontcombobox))

HEADERS	    =   mainwindow.h \
		FlowChartScene.h \
		FlowChartView.h \
		FlowEdge.h \
		FlowNode.h
SOURCES	    =   mainwindow.cpp \
		FlowChartScene.cpp \
		FlowChartView.cpp \
		FlowEdge.cpp \
		FlowNode.cpp \
		main.cpp
RESOURCES   =	diagramscene.qrc


# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/diagramscene
INSTALLS += target
