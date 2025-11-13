#pragma once

#include <QWidget>
#include <ols.hpp>

#define GREY_COLOR_BACKGROUND 0xFF4C4C4C

class OLSQTDisplay : public QWidget {
	Q_OBJECT
	Q_PROPERTY(QColor displayBackgroundColor MEMBER backgroundColor READ
			   GetDisplayBackgroundColor WRITE
				   SetDisplayBackgroundColor)

public:
	OLSQTDisplay(QWidget *parent = nullptr,
		     Qt::WindowFlags flags = Qt::WindowFlags());
	~OLSQTDisplay() {  }

	uint32_t backgroundColor = GREY_COLOR_BACKGROUND;

	QColor GetDisplayBackgroundColor() const;
	void SetDisplayBackgroundColor(const QColor &color);
	void UpdateDisplayBackgroundColor();

};
