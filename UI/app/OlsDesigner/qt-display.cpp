#include "qt-display.h"
#include "qt-wrappers.h"
#include "display-helpers.hpp"
#include <QWindow>
#include <QScreen>
#include <QResizeEvent>
#include <QShowEvent>
#include <ols-config.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif


static inline long long color_to_int(const QColor &color)
{
	auto shift = [&](unsigned val, int shift) {
		return ((val & 0xff) << shift);
	};

	return shift(color.red(), 0) | shift(color.green(), 8) |
	       shift(color.blue(), 16) | shift(color.alpha(), 24);
}

static inline QColor rgba_to_color(uint32_t rgba)
{
	return QColor::fromRgb(rgba & 0xFF, (rgba >> 8) & 0xFF,
			       (rgba >> 16) & 0xFF, (rgba >> 24) & 0xFF);
}

OLSQTDisplay::OLSQTDisplay(QWidget *parent, Qt::WindowFlags flags)
	: QWidget(parent, flags)
{
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_StaticContents);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_DontCreateNativeAncestors);
	setAttribute(Qt::WA_NativeWindow);

}

QColor OLSQTDisplay::GetDisplayBackgroundColor() const
{
	return rgba_to_color(backgroundColor);
}

void OLSQTDisplay::SetDisplayBackgroundColor(const QColor &color)
{
	uint32_t newBackgroundColor = (uint32_t)color_to_int(color);

	if (newBackgroundColor != backgroundColor) {
		backgroundColor = newBackgroundColor;
		UpdateDisplayBackgroundColor();
	}
}

void OLSQTDisplay::UpdateDisplayBackgroundColor()
{
	//obs_display_set_background_color(display, backgroundColor);
}




