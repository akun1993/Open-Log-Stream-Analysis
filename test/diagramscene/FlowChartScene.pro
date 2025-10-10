QT += widgets
requires(qtConfig(fontcombobox))

TEMPLATE = app

HEADERS	    =   \
		AdvancedToolBox.h \
		DesignWid.h \
		FlowChartScene.h \
		FlowChartView.h \
		FlowEdge.h \
		FlowNode.h
SOURCES	    =   \
		AdvancedToolBox.cpp \
		DesignWid.cpp \
		FlowChartScene.cpp \
		FlowChartView.cpp \
		FlowEdge.cpp \
		FlowNode.cpp \
		main.cpp
RESOURCES   =	diagramscene.qrc


# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/diagramscene
INSTALLS += target
