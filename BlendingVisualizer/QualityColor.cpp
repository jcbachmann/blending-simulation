#include "QualityColor.h"

constexpr const double DRAW_COLOR_MIN = 0.0;
constexpr const double DRAW_COLOR_MAX = 230.0;
constexpr const double DRAW_COLOR_MID = 0.5 * (DRAW_COLOR_MAX + DRAW_COLOR_MIN);

std::tuple<double, double, double> hsvToRgb(double h, double s, double v)
{
	if (s <= 0.0) {
		return std::make_tuple(v, v, v);
	}

	double hh = h;
	if (hh >= 360.0) {
		hh = 0.0;
	}
	hh /= 60.0;

	long i = (long) hh;
	double ff = hh - i;
	double p = v * (1.0 - s);
	double q = v * (1.0 - (s * ff));
	double t = v * (1.0 - (s * (1.0 - ff)));
	double r, g, b;

	switch (i) {
		case 0:
			r = v;
			g = t;
			b = p;
			break;

		case 1:
			r = q;
			g = v;
			b = p;
			break;

		case 2:
			r = p;
			g = v;
			b = t;
			break;

		case 3:
			r = p;
			g = q;
			b = v;
			break;

		case 4:
			r = t;
			g = p;
			b = v;
			break;
		case 5:

		default:
			r = v;
			g = p;
			b = q;
			break;
	}

	return std::make_tuple(r, g, b);
}

double mapRange(double fromMin, double fromMax, double toMin, double toMax, double value)
{
	return (toMax - toMin) * (value - fromMin) / (fromMax - fromMin) + toMin;
}

double qualityHue(double quality)
{
	const double QUALITY_MIN = 22.1;
	const double QUALITY_MAX = 40.6;
	return mapRange(QUALITY_MIN, QUALITY_MAX, DRAW_COLOR_MIN, DRAW_COLOR_MAX, quality);
}

double qualityHue(int amountLow, int amountMid, int amountHigh)
{
	int total = amountLow + amountMid + amountHigh;
	if (total <= 0) {
		return DRAW_COLOR_MIN;
	}
	return (double(amountLow) * DRAW_COLOR_MIN + double(amountMid) * DRAW_COLOR_MID + double(amountHigh) * DRAW_COLOR_MAX) / double(total);
}

std::tuple<double, double, double> qualityColor(float volume1, float volume2, float volume3)
{
	if (volume1 > volume2 && volume1 > volume3) {
		// Red
		return std::make_tuple(239.0 / 255.0, 115.0 / 255.0, 51.0 / 255.0);
	} else {
		if (volume2 > volume3) {
			// Blue
			return std::make_tuple(94.0 / 255.0, 149.0 / 255.0, 188.0 / 255.0);
		} else {
			// Yellow
			return std::make_tuple(206.0 / 255.0, 222.0 / 255.0, 27.0 / 255.0);
		}
	}
}
