#ifndef BLENDINGSIMULATOR_CONTINUOUSDROPS_H
#define BLENDINGSIMULATOR_CONTINUOUSDROPS_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace blendingsimulator
{
// One drop of a material flowing while the stacker moves: position, volume and the material portion it belongs to
struct ContinuousDrop
{
	double x;
	double z;
	double volume;
	int portion;
};

// Drops for a stacker that moves while the material flows: portion i flows evenly from portionTimes[i] to portionTimes[i + 1] (a portion of
// volume 0 is a gap), while the stacker follows the piecewise linear path (pathTimes, pathX, pathZ). The path is cut exactly where it
// crosses a cell boundary of the grid (cellX by cellZ) or a portion starts or ends; every piece gets the volume that flows during it, and
// consecutive pieces in the same cell and portion become one drop at their volume-weighted mean position. Every cell along the path gets
// the volume of the time the stacker spends over it, the first and last cell of a pass only the share of their overlap, and a stacker
// standing still makes one drop per portion. Same results as bmh.helpers.continuous_drops (numpy).
inline std::vector<ContinuousDrop> continuousDrops(const std::vector<double>& volumes, const std::vector<double>& portionTimes,
	const std::vector<double>& pathTimes, const std::vector<double>& pathX, const std::vector<double>& pathZ, double cellX, double cellZ)
{
	const std::size_t n = volumes.size();
	if (portionTimes.size() != n + 1 || pathX.size() != pathTimes.size() || pathZ.size() != pathTimes.size() || pathTimes.empty()) {
		throw std::invalid_argument("need n + 1 portion times for n portions and one x and z per path time");
	}
	const double start = portionTimes.front();
	const double end = portionTimes.back();

	// Position on the path at time t, constant beyond its ends (like numpy.interp)
	auto at = [&](const std::vector<double>& values, double t) {
		if (t <= pathTimes.front()) return values.front();
		if (t >= pathTimes.back()) return values.back();
		const std::size_t k = std::size_t(std::upper_bound(pathTimes.begin(), pathTimes.end(), t) - pathTimes.begin());
		const double t0 = pathTimes[k - 1], t1 = pathTimes[k];
		return values[k - 1] + (values[k] - values[k - 1]) * (t - t0) / (t1 - t0);
	};

	std::vector<double> times;
	times.reserve(pathTimes.size() + portionTimes.size());
	for (double t : pathTimes) {
		if (t >= start && t <= end) times.push_back(t);
	}
	times.insert(times.end(), portionTimes.begin(), portionTimes.end());
	std::sort(times.begin(), times.end());
	times.erase(std::unique(times.begin(), times.end()), times.end());

	// Cell boundaries crossed between neighbouring times, where the path is linear
	std::vector<double> cuts(times);
	for (std::size_t k = 0; k + 1 < times.size(); k++) {
		const double a = times[k], b = times[k + 1];
		for (int axis = 0; axis < 2; axis++) {
			const std::vector<double>& values = axis == 0 ? pathX : pathZ;
			const double c = axis == 0 ? cellX : cellZ;
			const double pa = at(values, a), pb = at(values, b);
			const double lo = std::min(pa, pb), hi = std::max(pa, pb);
			if (pa == pb || std::ceil(hi / c) - std::floor(lo / c) < 2.0) continue;
			for (double m = std::floor(lo / c) + 1.0; m < std::ceil(hi / c); m += 1.0) {
				cuts.push_back(a + (m * c - pa) / (pb - pa) * (b - a));
			}
		}
	}
	std::sort(cuts.begin(), cuts.end());
	cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());

	std::vector<ContinuousDrop> drops;
	std::int64_t lastCell = 0;
	int lastPortion = -1;
	double firstX = 0.0, firstZ = 0.0, sumX = 0.0, sumZ = 0.0;
	auto close = [&]() {
		if (lastPortion >= 0) {
			ContinuousDrop& d = drops.back();
			d.x = firstX + sumX / d.volume;
			d.z = firstZ + sumZ / d.volume;
		}
	};
	for (std::size_t k = 0; k + 1 < cuts.size(); k++) {
		const double mid = 0.5 * (cuts[k] + cuts[k + 1]);
		const int portion = std::max(0, std::min(int(std::upper_bound(portionTimes.begin(), portionTimes.end(), mid) - portionTimes.begin()) - 1, int(n) - 1));
		const double duration = portionTimes[portion + 1] - portionTimes[portion];
		const double rate = duration > 0.0 ? volumes[portion] / duration : 0.0;
		const double piece = (cuts[k + 1] - cuts[k]) * rate;
		if (!(piece > 0.0)) continue;
		const double x = at(pathX, mid), z = at(pathZ, mid);
		const std::int64_t cell = std::int64_t(std::floor(x / cellX)) * 1000003 + std::int64_t(std::floor(z / cellZ));
		if (portion != lastPortion || cell != lastCell) {
			close();
			drops.push_back({x, z, 0.0, portion});
			firstX = x;
			firstZ = z;
			sumX = 0.0;
			sumZ = 0.0;
			lastCell = cell;
			lastPortion = portion;
		}
		// relative to the drop's first position: pieces sharing one position (e.g. on a cell boundary) keep it exactly
		drops.back().volume += piece;
		sumX += piece * (x - firstX);
		sumZ += piece * (z - firstZ);
	}
	close();
	return drops;
}
}

#endif
