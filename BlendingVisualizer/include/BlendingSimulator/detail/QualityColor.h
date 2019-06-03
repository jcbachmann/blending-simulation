#ifndef QualityColorH
#define QualityColorH

#include <tuple>

namespace blendingsimulator
{
std::tuple<double, double, double> hsvToRgb(double h, double s, double v);
double mapRange(double fromMin, double fromMax, double toMin, double toMax, double value);
double qualityHue(double quality);
double qualityHue(int amountLow, int amountMid, int amountHigh);
std::tuple<double, double, double> qualityColor(float volume1, float volume2, float volume3);
}

#endif
