// Calibration and benchmark of the detailed simulation. Prints one JSON object per run.
//
//   DemCalibration cone <volume m³> <ppm3> [key=value ...]     a pile poured at one point: angle of repose, packing, time
//   DemCalibration chevron <passes> <ppm3> [key=value ...]     the bmh_ml bed (59 m x 20 m, 2,500 m³) stacked in Chevron passes
//
// key=value sets a DetailedParameters field: flowRate, dischargeHeight, beltSpeed, packingFraction, sizeSpread, friction,
// rollingFriction, restitution, youngsModulus, timeStepFactor, threads.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "BlendingSimulator/BlendingSimulatorDetailed.h"
#include "BlendingSimulator/ParticleParameters.h"

namespace bs = blendingsimulator;
namespace dem = blendingsimulator::dem;
using Simulator = bs::BlendingSimulatorDetailed<bs::AveragedParameters>;

namespace
{
constexpr double pi = 3.141592653589793238463;

void setParameter(bs::SimulationParameters& p, const std::string& assignment)
{
	const auto eq = assignment.find('=');
	if (eq == std::string::npos) {
		throw std::invalid_argument("expected key=value: " + assignment);
	}
	const std::string key = assignment.substr(0, eq);
	const double value = std::stod(assignment.substr(eq + 1));
	bs::DetailedParameters& d = p.detailed;
	if (key == "seed") p.seed = std::uint32_t(value);
	else if (key == "flowRate") d.flowRate = value;
	else if (key == "dischargeHeight") d.dischargeHeight = value;
	else if (key == "beltSpeed") d.beltSpeed = value;
	else if (key == "packingFraction") d.packingFraction = value;
	else if (key == "sizeSpread") d.sizeSpread = value;
	else if (key == "friction") d.friction = value;
	else if (key == "rollingFriction") d.rollingFriction = value;
	else if (key == "restitution") d.restitution = value;
	else if (key == "youngsModulus") d.youngsModulus = value;
	else if (key == "timeStepFactor") d.timeStepFactor = value;
	else if (key == "threads") d.threads = int(value);
	else if (key == "restVelocity") d.restVelocity = value;
	else if (key == "restTime") d.restTime = value;
	else if (key == "skinFactor") d.skinFactor = value;
	else throw std::invalid_argument("unknown parameter " + key);
}

// Heights of the particle tops on a fine grid
struct Surface
{
	double cell;
	int nx;
	int nz;
	std::vector<double> h;

	Surface(const dem::World& world, double sizeX, double sizeZ, double cell)
		: cell(cell)
		, nx(int(std::ceil(sizeX / cell)))
		, nz(int(std::ceil(sizeZ / cell)))
		, h(std::size_t(nx) * nz, 0.0)
	{
		for (std::size_t i = 0; i < world.particleCount(); i++) {
			if (world.getState(int(i)) == dem::World::State::Removed) {
				continue;
			}
			const dem::Vec3& c = world.getPosition(int(i));
			const double r = world.getRadius(int(i));
			for (int zi = std::max(0, int((c.z - r) / cell)); zi <= std::min(nz - 1, int((c.z + r) / cell)); zi++) {
				for (int xi = std::max(0, int((c.x - r) / cell)); xi <= std::min(nx - 1, int((c.x + r) / cell)); xi++) {
					const double dx = (xi + 0.5) * cell - c.x;
					const double dz = (zi + 0.5) * cell - c.z;
					const double d2 = r * r - dx * dx - dz * dz;
					if (d2 >= 0.0) {
						double& v = h[std::size_t(zi) * nx + xi];
						v = std::max(v, c.y + std::sqrt(d2));
					}
				}
			}
		}
	}

	double volume() const
	{
		double v = 0.0;
		for (double x : h) {
			v += x;
		}
		return v * cell * cell;
	}
};

double cross(const std::pair<double, double>& o, const std::pair<double, double>& a, const std::pair<double, double>& b)
{
	return (a.first - o.first) * (b.second - o.second) - (a.second - o.second) * (b.first - o.first);
}

// Area of the convex hull of points (monotone chain)
double hullArea(std::vector<std::pair<double, double>> p)
{
	if (p.size() < 3) {
		return 0.0;
	}
	std::sort(p.begin(), p.end());
	std::vector<std::pair<double, double>> hull(2 * p.size());
	std::size_t k = 0;
	for (std::size_t i = 0; i < p.size(); i++) {
		while (k >= 2 && cross(hull[k - 2], hull[k - 1], p[i]) <= 0) k--;
		hull[k++] = p[i];
	}
	for (std::size_t i = p.size() - 1, t = k + 1; i > 0; i--) {
		while (k >= t && cross(hull[k - 2], hull[k - 1], p[i - 1]) <= 0) k--;
		hull[k++] = p[i - 1];
	}
	hull.resize(k - 1);
	double a = 0.0;
	for (std::size_t i = 0; i < hull.size(); i++) {
		const auto& u = hull[i];
		const auto& v = hull[(i + 1) % hull.size()];
		a += u.first * v.second - v.first * u.second;
	}
	return 0.5 * std::abs(a);
}

// Angle of repose like the simulator comparison: the radius of the circle with the area of the pile above a height falls with the slope
// of the cone; fitted between 20 % and 80 % of the peak (the 99th percentile of the occupied cells)
double coneAngle(const Surface& s)
{
	std::vector<double> occupied;
	for (double v : s.h) {
		if (v > 0.0) occupied.push_back(v);
	}
	if (occupied.size() < 10) return std::nan("");
	std::sort(occupied.begin(), occupied.end());
	const double peak = occupied[std::size_t(0.99 * (occupied.size() - 1))];
	std::vector<double> xs, ys;
	for (int k = 1; k < 50; k++) {
		const double level = peak * k / 50.0;
		if (level < 0.2 * peak || level > 0.8 * peak) continue;
		std::vector<std::pair<double, double>> points;
		for (int zi = 0; zi < s.nz; zi++) {
			for (int xi = 0; xi < s.nx; xi++) {
				if (s.h[std::size_t(zi) * s.nx + xi] > level) points.emplace_back((xi + 0.5) * s.cell, (zi + 0.5) * s.cell);
			}
		}
		const double radius = std::sqrt(hullArea(points) / pi);
		if (radius > 0.0) {
			xs.push_back(radius);
			ys.push_back(level);
		}
	}
	const double n = double(xs.size());
	double sx = 0, sy = 0, sxx = 0, sxy = 0;
	for (std::size_t i = 0; i < xs.size(); i++) {
		sx += xs[i];
		sy += ys[i];
		sxx += xs[i] * xs[i];
		sxy += xs[i] * ys[i];
	}
	const double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);
	return std::atan(-slope) * 180.0 / pi;
}

double solidVolume(const dem::World& w)
{
	double v = 0.0;
	for (std::size_t i = 0; i < w.particleCount(); i++) {
		if (w.getState(int(i)) != dem::World::State::Removed) {
			const double r = w.getRadius(int(i));
			v += 4.0 / 3.0 * pi * r * r * r;
		}
	}
	return v;
}

std::size_t countFixed(const dem::World& w)
{
	std::size_t n = 0;
	for (std::size_t i = 0; i < w.particleCount(); i++) {
		n += w.getState(int(i)) == dem::World::State::Fixed;
	}
	return n;
}
}

int main(int argc, char** argv)
{
	if (argc < 4) {
		std::fprintf(stderr, "usage: %s cone|chevron <volume|passes> <ppm3> [key=value ...]\n", argv[0]);
		return 1;
	}
	const std::string mode = argv[1];
	const double amount = std::atof(argv[2]);
	bs::SimulationParameters p;
	p.particlesPerCubicMeter = float(std::atof(argv[3]));
	p.seed = 1;
	std::string args;
	for (int i = 4; i < argc; i++) {
		setParameter(p, argv[i]);
		args += (args.empty() ? "" : " ") + std::string(argv[i]);
	}

	const auto start = std::chrono::steady_clock::now();
	std::size_t maxDynamic = 0;
	double volume = 0.0;
	if (mode == "cone") {
		const double side = 2.0 * std::cbrt(3.0 * amount / pi) + 6.0; // a 45° cone of this volume with room around it
		p.heapWorldSizeX = float(side);
		p.heapWorldSizeZ = float(side);
		volume = amount;
	} else if (mode == "chevron") {
		p.heapWorldSizeX = 59.0f;
		p.heapWorldSizeZ = 20.0f;
		volume = 2500.0;
	} else {
		std::fprintf(stderr, "unknown mode %s\n", mode.c_str());
		return 1;
	}
	Simulator simulator(p);
	const double step = 0.25; // m³ per stack call: the stacker moves continuously
	const int calls = int(volume / step + 0.5);
	for (int k = 0; k < calls; k++) {
		const double t = (k + 0.5) / calls;
		double x;
		if (mode == "cone") {
			x = 0.5 * p.heapWorldSizeX;
		} else {
			const double passes = amount;
			const double u = t * passes;
			const double frac = u - std::floor(u);
			const bool forward = int(std::floor(u)) % 2 == 0;
			x = 10.0 + 39.0 * (forward ? frac : 1.0 - frac);
		}
		simulator.stack(float(x), 0.5f * p.heapWorldSizeZ, bs::AveragedParameters(step, {t}));
		maxDynamic = std::max(maxDynamic, simulator.getWorld().dynamicCount());
	}
	simulator.finishStacking();
	const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

	const dem::World& world = simulator.getWorld();
	const Surface surface(world, p.heapWorldSizeX, p.heapWorldSizeZ, 0.05);
	const double bulk = surface.volume();
	const double solid = solidVolume(world);

	// Reclaim in 1 m slices: quality spread (F1 of a linear input) and the volume profile for the chevron
	std::vector<double> sliceVolume;
	std::vector<double> sliceQuality;
	for (float position = 0.0f; !simulator.reclaimingFinished(); position += 1.0f) {
		const bs::AveragedParameters slice = simulator.reclaim(position);
		sliceVolume.push_back(slice.getVolume());
		sliceQuality.push_back(slice.getVolume() > 0 ? slice.getValues()[0] : 0.0);
	}
	double total = 0.0;
	double mean = 0.0;
	for (std::size_t i = 0; i < sliceVolume.size(); i++) {
		total += sliceVolume[i];
		mean += sliceVolume[i] * sliceQuality[i];
	}
	mean /= total;
	double f1 = 0.0;
	for (std::size_t i = 0; i < sliceVolume.size(); i++) {
		f1 += sliceVolume[i] * (sliceQuality[i] - mean) * (sliceQuality[i] - mean);
	}
	f1 = std::sqrt(f1 / total);

	std::printf("{\"mode\": \"%s\", \"amount\": %g, \"ppm3\": %g, \"particles\": %zu, \"maxDynamic\": %zu, \"fixed\": %zu, \"seconds\": %.2f, "
		"\"simulatedSeconds\": %.1f, \"blockedSeconds\": %.2f, \"timeStep\": %.6f, \"angle\": %.2f, \"bulkVolume\": %.1f, "
		"\"stackedVolume\": %.1f, \"packing\": %.4f, \"lostVolume\": %.2f, \"reclaimedVolume\": %.2f, \"F1\": %.5f, \"threads\": %d, "
		"\"rollingFriction\": %g, \"friction\": %g, \"packingFraction\": %g, \"settled\": %d, \"args\": \"%s\", \"volumes\": [",
		mode.c_str(), amount, p.particlesPerCubicMeter, world.particleCount(), maxDynamic, countFixed(world), seconds, world.time(),
		simulator.getBlockedTime(), world.timeStep(), mode == "cone" ? coneAngle(surface) : std::nan(""), bulk, volume, solid / bulk,
		simulator.getLostVolume(), total, f1, p.detailed.threads, p.detailed.rollingFriction, p.detailed.friction, p.detailed.packingFraction,
		int(world.settled()), args.c_str());
	for (std::size_t i = 0; i < sliceVolume.size(); i++) {
		std::printf("%s%.3f", i ? ", " : "", sliceVolume[i]);
	}
	const dem::World::Profile& prof = world.getProfile();
	std::printf("], \"profile\": {\"steps\": %llu, \"pairUpdates\": %llu, \"meanDynamic\": %.1f, \"meanPairs\": %.1f, \"pairCheck\": %.2f, "
		"\"pairSearch\": %.2f, \"pairMerge\": %.2f, \"entries\": %.2f, \"forces\": %.2f, \"wakes\": %.2f, \"maintenance\": %.2f}}\n",
		(unsigned long long)prof.steps, (unsigned long long)prof.pairUpdates, prof.dynamicSum / double(std::max<std::uint64_t>(1, prof.steps)),
		prof.pairSum / double(std::max<std::uint64_t>(1, prof.steps)), prof.pairCheck, prof.pairSearch, prof.pairMerge, prof.entries, prof.forces,
		prof.wakes, prof.maintenance);
	return 0;
}
