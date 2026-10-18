#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace blendingsimulator
{
namespace formula
{
// Antiderivative of sqrt(s^2 - m^2) in s for s >= m (constant below)
inline double sqrtIntegral(double s, double m)
{
	if (s <= m) {
		return m > 0.0 ? -0.5 * m * m * std::log(m) : 0.0;
	}
	const double w = std::sqrt(s * s - m * m);
	return 0.5 * (s * w - (m > 0.0 ? m * m * std::log(s + w) : 0.0));
}

// Height interval within [a, b] where the section of the cone (H, m) is wider than that of (Hj, mj): their squared widths differ by
// (H - Hj)(H + Hj - 2 y) + mj^2 - m^2, linear in y. Returns false if it is empty.
inline bool dominates(double H, double m, double Hj, double mj, double a, double b, double& lo, double& hi)
{
	if (b <= a) {
		return false;
	}
	const double dH = H - Hj;
	const double c0 = mj * mj - m * m;
	lo = a;
	hi = b;
	if (std::abs(dH) < 1e-13) {
		return c0 > 0.0;
	}
	const double crossing = 0.5 * (H + Hj) + 0.5 * c0 / dH;
	if (dH > 0.0) {
		hi = std::min(b, crossing);
	} else {
		lo = std::max(a, crossing);
	}
	return hi > lo;
}
}

template<typename Parameters>
BlendingSimulatorFormula<Parameters>::BlendingSimulatorFormula(SimulationParameters simulationParameters, double angleOfRepose)
	: BlendingSimulator<Parameters>(simulationParameters)
{
	if (simulationParameters.circular) {
		throw std::invalid_argument(
			"BlendingSimulatorFormula does not support circular beds: the formula needs a straight stacker line, use BlendingSimulatorCone");
	}
	const double degrees = std::atan(1.0) * 4.0 / 180.0;
	slope = std::tan(angleOfRepose * degrees);
	verticalReclaim = std::abs(90.0f - simulationParameters.reclaimAngle) < 0.01f;
	horizontalReclaim = std::abs(simulationParameters.reclaimAngle) < 0.01f;
	tanReclaimAngle = std::tan(simulationParameters.reclaimAngle * degrees);
	inverseTanReclaimAngle = verticalReclaim || horizontalReclaim ? 0.0 : 1.0 / tanReclaimAngle;

	// Cells of about the particle size of the other simulators, 1 / ppm3^(1/3), adjusted so that they tile the bed exactly
	const double size = 1.0 / std::cbrt(double(simulationParameters.particlesPerCubicMeter));
	nx = std::max(1, int(std::lround(simulationParameters.heapWorldSizeX / size)));
	nz = std::max(1, int(std::lround(simulationParameters.heapWorldSizeZ / size)));
	cellX = simulationParameters.heapWorldSizeX / nx;
	cellZ = simulationParameters.heapWorldSizeZ / nz;
	pieces.assign(std::size_t(nx) * maxPieces, Piece{0.0, 0.0, 0.0, 0.0});
	pieceCount.assign(std::size_t(nx), 0);
	ridge.assign(std::size_t(nx), 0.0);
	shell.assign(std::size_t(nx), 0.0);
	this->initializeHeapMap((unsigned int)nx, (unsigned int)nz);
	slices.resize((unsigned int)nx, float(cellX));

	clear();
}

template<typename Parameters>
void BlendingSimulatorFormula<Parameters>::clear()
{
	std::fill(pieceCount.begin(), pieceCount.end(), 0);
	std::fill(ridge.begin(), ridge.end(), 0.0);
	std::fill(shell.begin(), shell.end(), 0.0);
	lineSet = false;
	lastX = -1e9;
	lastVolume = 0.0;
	lastExcess = 0.0;
	lastApex = 0.0;
	slices.clear();
}

template<typename Parameters>
void BlendingSimulatorFormula<Parameters>::finishStacking()
{
	// Nothing to do
}

template<typename Parameters>
bool BlendingSimulatorFormula<Parameters>::reclaimingFinished()
{
	return slices.finished();
}

template<typename Parameters>
Parameters BlendingSimulatorFormula<Parameters>::reclaim(float position)
{
	return slices.reclaim(position);
}

template<typename Parameters>
void BlendingSimulatorFormula<Parameters>::stack(float x, float z, const Parameters& parameters)
{
	drop(x, z, parameters);
}

template<typename Parameters>
void BlendingSimulatorFormula<Parameters>::stackSingle(float x, float z, const Parameters& parameters)
{
	drop(x, z, parameters);
}

template<typename Parameters>
double BlendingSimulatorFormula<Parameters>::getCellSizeX() const
{
	return cellX;
}

template<typename Parameters>
double BlendingSimulatorFormula<Parameters>::getLastApex() const
{
	return lastApex;
}

template<typename Parameters>
double BlendingSimulatorFormula<Parameters>::getHeight(double x, double z) const
{
	if (!lineSet) {
		return 0.0;
	}
	// The surface at distance d from the line is the highest cone there: max_j H_j - sqrt(m_j^2 + (k d)^2) over the cones of the envelope
	const int i = std::max(0, std::min(int(std::floor(x / cellX)), nx - 1));
	const double kd = slope * std::abs(z - lineZ);
	const Piece* p = &pieces[std::size_t(i) * maxPieces];
	double height = 0.0;
	for (int j = 0; j < pieceCount[i]; j++) {
		height = std::max(height, p[j].H - std::sqrt(p[j].m * p[j].m + kd * kd));
	}
	return height;
}

template<typename Parameters>
void BlendingSimulatorFormula<Parameters>::updateHeapMap()
{
	for (int j = 0; j < nz; j++) {
		for (int i = 0; i < nx; i++) {
			this->heapMap[std::size_t(j) * nx + i] = float(getHeight((i + 0.5) * cellX, (j + 0.5) * cellZ));
		}
	}
}

template<typename Parameters>
double BlendingSimulatorFormula<Parameters>::sectionIntegral(double H, double m, double top, double c, double ya, double yb, double* dH) const
{
	const double k = slope;
	yb = std::min(yb, top);
	ya = std::max(ya, 0.0);
	if (dH) {
		*dH = 0.0;
	}
	if (yb <= ya) {
		return 0.0;
	}
	// Clamped at the wall below yc
	const double yc = H - std::sqrt(m * m + k * k * c * c);
	const double clamped = std::min(yb, std::max(ya, yc));
	double sum = c * (clamped - ya);
	if (yb > clamped) {
		const double sLow = H - clamped;
		const double sHigh = H - yb;
		sum += (formula::sqrtIntegral(sLow, m) - formula::sqrtIntegral(sHigh, m)) / k;
		if (dH) {
			*dH = (std::sqrt(std::max(sLow * sLow - m * m, 0.0)) - std::sqrt(std::max(sHigh * sHigh - m * m, 0.0))) / k;
		}
	}
	return sum;
}

template<typename Parameters>
template<typename Visit>
double BlendingSimulatorFormula<Parameters>::cellShell(int i, double H, double m, double* dH, Visit&& visit) const
{
	const Piece* p = &pieces[std::size_t(i) * maxPieces];
	const int count = pieceCount[i];
	const double top = H - m;
	if (top <= 0.0) {
		if (dH) {
			*dH = 0.0;
		}
		return 0.0;
	}
	// Symmetric walls: one side, doubled
	const bool symmetric = wall1 == wall2;
	const int sides = symmetric ? 1 : 2;
	const double weight = symmetric ? 2.0 : 1.0;
	const double walls[2] = {wall1, wall2};
	double sum = 0.0;
	double derivative = 0.0;
	auto add = [&](double lo, double hi, double Hj, double mj, bool old) {
		double area = 0.0;
		for (int side = 0; side < sides; side++) {
			double d;
			const double below = old ? sectionIntegral(Hj, mj, Hj - mj, walls[side], lo, hi, nullptr) : 0.0;
			area += weight * (sectionIntegral(H, m, top, walls[side], lo, hi, &d) - below);
			derivative += weight * d;
		}
		if (area > 0.0) {
			sum += area;
			visit(lo, hi, Hj, mj, old, area);
		}
	};
	double covered = 0.0;
	for (int j = 0; j < count; j++) {
		covered = std::max(covered, p[j].yb);
		if (p[j].ya >= top) {
			break;
		}
		double lo, hi;
		if (formula::dominates(H, m, p[j].H, p[j].m, p[j].ya, std::min(p[j].yb, top), lo, hi)) {
			add(lo, hi, p[j].H, p[j].m, true);
		}
	}
	// Above the old pile
	if (top > covered) {
		add(covered, top, 0.0, 0.0, false);
	}
	if (dH) {
		*dH = derivative;
	}
	return sum;
}

template<typename Parameters>
void BlendingSimulatorFormula<Parameters>::updateEnvelope(int i, double H, double m)
{
	Piece* p = &pieces[std::size_t(i) * maxPieces];
	const int count = pieceCount[i];
	const double top = H - m;
	// Each old piece splits into at most three, plus the part above the old pile
	Piece out[3 * maxPieces + 1];
	int n = 0;
	auto put = [&](double Hp, double mp, double a, double b) {
		if (b - a <= 1e-12) {
			return;
		}
		if (n > 0 && out[n - 1].H == Hp && out[n - 1].m == mp && std::abs(out[n - 1].yb - a) < 1e-12) {
			out[n - 1].yb = b;
			return;
		}
		out[n++] = Piece{Hp, mp, a, b};
	};
	double covered = 0.0;
	for (int j = 0; j < count; j++) {
		covered = std::max(covered, p[j].yb);
		double lo, hi;
		if (p[j].ya < top && formula::dominates(H, m, p[j].H, p[j].m, p[j].ya, std::min(p[j].yb, top), lo, hi)) {
			put(p[j].H, p[j].m, p[j].ya, lo);
			put(H, m, lo, hi);
			put(p[j].H, p[j].m, hi, p[j].yb);
		} else {
			put(p[j].H, p[j].m, p[j].ya, p[j].yb);
		}
	}
	if (top > covered) {
		put(H, m, covered, top);
	}
	while (n > maxPieces) {
		int shortest = 0;
		for (int j = 1; j < n; j++) {
			if (out[j].yb - out[j].ya < out[shortest].yb - out[shortest].ya) {
				shortest = j;
			}
		}
		const int other = shortest == 0 ? 1 : shortest - 1;
		out[other].ya = std::min(out[other].ya, out[shortest].ya);
		out[other].yb = std::max(out[other].yb, out[shortest].yb);
		for (int j = shortest; j + 1 < n; j++) {
			out[j] = out[j + 1];
		}
		n--;
	}
	std::copy(out, out + n, p);
	pieceCount[i] = n;
}

template<typename Parameters>
double BlendingSimulatorFormula<Parameters>::band(double H, double m, double Hj, double mj, bool old, double ya, double yb) const
{
	const bool symmetric = wall1 == wall2;
	const int sides = symmetric ? 1 : 2;
	const double weight = symmetric ? 2.0 : 1.0;
	const double walls[2] = {wall1, wall2};
	double area = 0.0;
	for (int side = 0; side < sides; side++) {
		const double below = old ? sectionIntegral(Hj, mj, Hj - mj, walls[side], ya, yb, nullptr) : 0.0;
		area += weight * (sectionIntegral(H, m, H - m, walls[side], ya, yb, nullptr) - below);
	}
	return area;
}

template<typename Parameters>
void BlendingSimulatorFormula<Parameters>::reclaimBand(double x, double ya, double yb, const Parameters& parameters, double volume)
{
	// Uniform in height between ya and yb and over the cell's length: a trapezoid of reclaim positions
	if (horizontalReclaim) {
		slices.push(0, parameters, volume);
		return;
	}
	slices.push(x - 0.5 * (ya + yb) * inverseTanReclaimAngle, cellX, (yb - ya) * std::abs(inverseTanReclaimAngle), parameters, volume);
}

template<typename Parameters>
void BlendingSimulatorFormula<Parameters>::drop(double x0, double z0, const Parameters& parameters)
{
	const double volume = parameters.getVolume();
	if (!(volume > 0.0)) {
		return;
	}
	if (!lineSet) {
		lineSet = true;
		lineZ = z0;
		wall1 = std::max(z0, 1e-9);
		wall2 = std::max(double(this->simulationParameters.heapWorldSizeZ) - z0, 1e-9);
	} else if (std::abs(z0 - lineZ) > 1e-4) {
		throw std::invalid_argument(
			"BlendingSimulatorFormula needs all drops on one line along the bed: a drop at z = " + std::to_string(z0) + " after drops at z = "
			+ std::to_string(lineZ) + " (lateral stacker motion is not supported, use BlendingSimulatorCone)");
	}
	x0 = std::max(0.0, std::min(x0, double(this->simulationParameters.heapWorldSizeX)));

	constexpr double pi = 3.14159265358979323846;
	const double k = slope;
	const double dx = cellX;
	const int ic = std::max(0, std::min(int(std::floor(x0 / dx)), nx - 1));
	int lo = ic + 1;
	int hi = ic;
	double held = 0.0;
	auto none = [](double, double, double, double, bool, double) {};

	// Shell volume for apex H and its derivative; lo..hi the cells that receive material
	auto volumeAt = [&](double H, double& derivative) {
		double sum = 0.0;
		derivative = 0.0;
		const double reach = std::min(H / k, double(nx) * dx);
		const int a = std::max(0, int(std::floor((x0 - reach) / dx)));
		const int b = std::min(nx - 1, int(std::floor((x0 + reach) / dx)));
		lo = b + 1;
		hi = a - 1;
		for (int i = a; i <= b; i++) {
			shell[i] = 0.0;
			const double m = k * std::abs((i + 0.5) * dx - x0);
			if (H - m <= 0.0) {
				continue;
			}
			double d;
			const double area = cellShell(i, H, m, &d, none);
			if (area <= 0.0) {
				continue;
			}
			shell[i] = area;
			sum += area;
			derivative += d;
			lo = std::min(lo, i);
			hi = std::max(hi, i);
		}
		held = sum * dx;
		return held;
	};

	// Start above the root where possible: the previous drop's apex excess over the ridge, scaled like a cone on a flat ridge (D ~ V^(2/3)),
	// or a cone on a flat ridge of height r, which holds V = (16 sqrt(2) / 9) (D r)^(3/2) / k^2 above it (D = H - r, small D), or a cone of
	// volume V on flat ground, V = pi H^3 / (3 k^2), whichever is larger
	const double ridgeHeight = std::max(ridge[ic], 1e-9);
	const double coneHeight = std::cbrt(3.0 * volume * k * k / pi);
	double lower = 0.0;
	double upper;
	if (lastVolume > 0.0 && std::abs(x0 - lastX) <= 1.5 * dx && volume < 10.0 * lastVolume && volume > 0.1 * lastVolume) {
		upper = ridgeHeight + std::max(1.02 * lastExcess * std::pow(volume / lastVolume, 2.0 / 3.0), 1e-6);
	} else {
		const double flatExcess = std::pow(9.0 * k * k * volume / (16.0 * std::sqrt(2.0)), 2.0 / 3.0) / ridgeHeight;
		upper = std::max(ridgeHeight + std::min(1.1 * flatExcess, coneHeight), coneHeight);
	}
	double derivative;
	double upperExcess = volumeAt(upper, derivative) - volume;
	if (upperExcess < 0.0 && derivative > 0.0) {
		// Below the root: one Newton step overshoots above it (the volume is convex in H)
		lower = upper;
		upper = upper - upperExcess / (derivative * dx);
		upperExcess = volumeAt(upper, derivative) - volume;
	}
	while (upperExcess < 0.0) {
		lower = upper;
		upper = ridgeHeight + 2.0 * std::max(upper - ridgeHeight, k * dx) + k * dx;
		upperExcess = volumeAt(upper, derivative) - volume;
	}

	// Newton from above, bisection where a step leaves the bracket
	double apex = upper;
	double excess = upperExcess;
	for (int iteration = 0; iteration < 100; iteration++) {
		if (std::abs(excess) <= tolerance * volume || upper - lower <= 1e-12) {
			break;
		}
		double next = derivative > 0.0 ? apex - excess / (derivative * dx) : 0.5 * (lower + upper);
		if (!(next > lower && next < upper)) {
			next = 0.5 * (lower + upper);
		}
		apex = next;
		excess = volumeAt(apex, derivative) - volume;
		if (excess < 0.0) {
			lower = apex;
		} else {
			upper = apex;
		}
	}
	if (excess < -tolerance * volume) {
		apex = upper;
		volumeAt(apex, derivative);
	}
	lastX = x0;
	lastVolume = volume;
	lastExcess = apex - ridgeHeight;
	lastApex = apex;

	// The shell holds the volume to the tolerance; scaled to hold it exactly
	const double scale = held > 0.0 ? volume / held : 0.0;
	for (int i = lo; i <= hi; i++) {
		if (shell[i] <= 0.0) {
			continue;
		}
		const double x = (i + 0.5) * dx;
		const double m = k * std::abs(x - x0);
		cellShell(i, apex, m, nullptr, [&](double ya, double yb, double Hj, double mj, bool old, double area) {
			if (yb - ya <= bandHeight) {
				reclaimBand(x, ya, yb, parameters, area * dx * scale);
				return;
			}
			const double ym = ya + 0.5 * (yb - ya);
			for (const auto& [y0, y1] : {std::pair<double, double>{ya, ym}, std::pair<double, double>{ym, yb}}) {
				const double part = band(apex, m, Hj, mj, old, y0, y1);
				if (part > 0.0) {
					reclaimBand(x, y0, y1, parameters, part * dx * scale);
				}
			}
		});
		updateEnvelope(i, apex, m);
		ridge[i] = std::max(ridge[i], apex - m);
	}
}
}
