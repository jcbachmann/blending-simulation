#include <algorithm>
#include <cmath>

template<typename Parameters>
blendingsimulator::BlendingSimulatorCone<Parameters>::BlendingSimulatorCone(SimulationParameters simulationParameters, double angleOfRepose)
	: BlendingSimulator<Parameters>(simulationParameters)
{
	const double degrees = std::atan(1.0) * 4.0 / 180.0;
	slope = std::tan(angleOfRepose * degrees);
	verticalReclaim = std::abs(90.0f - simulationParameters.reclaimAngle) < 0.01f;
	horizontalReclaim = std::abs(simulationParameters.reclaimAngle) < 0.01f;
	tanReclaimAngle = std::tan(simulationParameters.reclaimAngle * degrees);

	// Cells of about the particle size of the other simulators, 1 / ppm3^(1/3), adjusted in x and z so that they tile the bed exactly
	const double size = 1.0 / std::cbrt(double(simulationParameters.particlesPerCubicMeter));
	nx = std::max(1, int(std::lround(simulationParameters.heapWorldSizeX / size)));
	nz = std::max(1, int(std::lround(simulationParameters.heapWorldSizeZ / size)));
	cellX = simulationParameters.heapWorldSizeX / nx;
	cellZ = simulationParameters.heapWorldSizeZ / nz;
	heights.assign(std::size_t(nx) * nz, 0.0);
	this->initializeHeapMap((unsigned int)nx, (unsigned int)nz);

	if (simulationParameters.circular) {
		// The circle of the fast simulation: radius a quarter of the smaller bed dimension around the bed center
		circumference = 2.0 * this->pi * 0.25 * std::min(simulationParameters.heapWorldSizeX, simulationParameters.heapWorldSizeZ);
		const int count = std::max(1, int(circumference / cellX + 0.5));
		slices.resize((unsigned int)count, float(circumference / count));
	} else {
		slices.resize((unsigned int)nx, float(cellX));
	}

	clear();
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorCone<Parameters>::clear()
{
	std::fill(heights.begin(), heights.end(), 0.0);
	slices.clear();
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorCone<Parameters>::finishStacking()
{
	// Nothing to do
}

template<typename Parameters>
bool blendingsimulator::BlendingSimulatorCone<Parameters>::reclaimingFinished()
{
	return slices.finished();
}

template<typename Parameters>
Parameters blendingsimulator::BlendingSimulatorCone<Parameters>::reclaim(float position)
{
	return slices.reclaim(position);
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorCone<Parameters>::stack(float x, float z, const Parameters& parameters)
{
	drop(x, z, parameters);
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorCone<Parameters>::stackSingle(float x, float z, const Parameters& parameters)
{
	drop(x, z, parameters);
}

template<typename Parameters>
double blendingsimulator::BlendingSimulatorCone<Parameters>::getCellSizeX() const
{
	return cellX;
}

template<typename Parameters>
double blendingsimulator::BlendingSimulatorCone<Parameters>::getCellSizeZ() const
{
	return cellZ;
}

template<typename Parameters>
double blendingsimulator::BlendingSimulatorCone<Parameters>::getHeight(double x, double z) const
{
	const int i = std::max(0, std::min(int(std::floor(x / cellX)), nx - 1));
	const int j = std::max(0, std::min(int(std::floor(z / cellZ)), nz - 1));
	return heights[std::size_t(i) * nz + j];
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorCone<Parameters>::gather(double x0, double z0, double bound)
{
	candidateBase.clear();
	candidateCell.clear();
	const double reach = bound / slope + 2.0 * std::max(cellX, cellZ);
	const int i0 = std::max(0, int(std::floor((x0 - reach) / cellX)));
	const int i1 = std::min(nx - 1, int(std::floor((x0 + reach) / cellX)));
	const int j0 = std::max(0, int(std::floor((z0 - reach) / cellZ)));
	const int j1 = std::min(nz - 1, int(std::floor((z0 + reach) / cellZ)));
	for (int i = i0; i <= i1; i++) {
		const double dx = (i + 0.5) * cellX - x0;
		const double slopeDx = slope * std::abs(dx);
		const double* row = &heights[std::size_t(i) * nz];
		// (restricting j to the disc of radius reach measured slower: the cheap test below rejects those cells faster)
		for (int j = j0; j <= j1; j++) {
			const double dz = (j + 0.5) * cellZ - z0;
			// The distance is at least the larger of |dx| and |dz|: most cells are rejected before the square root
			if (row[j] + std::max(slopeDx, slope * std::abs(dz)) >= bound) {
				continue;
			}
			const double base = row[j] + slope * std::sqrt(dx * dx + dz * dz);
			if (base < bound) {
				candidateBase.push_back(base);
				candidateCell.push_back(i * nz + j);
			}
		}
	}
}

template<typename Parameters>
double blendingsimulator::BlendingSimulatorCone<Parameters>::solveApex(double apex, double volume)
{
	// Newton's method from above on the volume between the cone and the pile, a convex, increasing, piecewise linear function of the apex
	// height: the steps decrease monotonically to the root, and the cells whose base lies above a step can never receive material, so
	// they leave the candidate lists for good. Exact after finitely many steps, the lists shrink to about the filled cells after the first.
	const double area = cellX * cellZ;
	std::size_t n = candidateBase.size();
	for (int step = 0; step < 200; step++) {
		double sum = 0.0;
		std::size_t kept = 0;
		for (std::size_t m = 0; m < n; m++) {
			const double thickness = apex - candidateBase[m];
			if (thickness > 0.0) {
				sum += thickness;
				candidateBase[kept] = candidateBase[m];
				candidateCell[kept] = candidateCell[m];
				kept++;
			}
		}
		n = kept;
		const double excess = sum * area - volume;
		if (excess <= 1e-12 * volume || n == 0) {
			break;
		}
		apex -= excess / (double(n) * area);
	}
	candidateBase.resize(n);
	candidateCell.resize(n);
	return apex;
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorCone<Parameters>::drop(double x0, double z0, const Parameters& parameters)
{
	const double volume = parameters.getVolume();
	if (!(volume > 0.0)) {
		return;
	}
	x0 = std::max(0.0, std::min(x0, double(this->simulationParameters.heapWorldSizeX)));
	z0 = std::max(0.0, std::min(z0, double(this->simulationParameters.heapWorldSizeZ)));

	// Upper bound of the apex: start a little above the cone of this volume on a flat surface at the drop cell, which bounds most drops,
	// and double the margin until the cells below the bound hold the volume (the pile's slope never exceeds the cones', so the bound of a
	// cone of twice the slope, cbrt(4) times as high, always suffices)
	const double coneHeight = std::cbrt(3.0 * volume * slope * slope / this->pi);
	const int ic = std::max(0, std::min(int(std::floor(x0 / cellX)), nx - 1));
	const int jc = std::max(0, std::min(int(std::floor(z0 / cellZ)), nz - 1));
	const double dropBase =
		heights[std::size_t(ic) * nz + jc] + slope * std::hypot((ic + 0.5) * cellX - x0, (jc + 0.5) * cellZ - z0);
	double bound = dropBase + 1.05 * coneHeight + 2.0 * slope * std::max(cellX, cellZ);
	while (true) {
		gather(x0, z0, bound);
		double held = 0.0;
		for (double base : candidateBase) {
			held += bound - base;
		}
		if (held * cellX * cellZ >= volume) {
			break;
		}
		bound = dropBase + 2.0 * (bound - dropBase);
	}

	const double apex = solveApex(bound, volume);
	for (std::size_t m = 0; m < candidateBase.size(); m++) {
		const double thickness = apex - candidateBase[m];
		if (thickness <= 0.0) {
			continue;
		}
		const int id = candidateCell[m];
		const double base = heights[id];
		heights[id] = base + thickness;
		reclaimShell(id / nz, id % nz, base, thickness, parameters);
	}
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorCone<Parameters>::reclaimShell(int i, int j, double base, double thickness, const Parameters& parameters)
{
	// The column of the cell from base to base + thickness, taken as a box, is cut by faces at the reclaim angle
	const double share = thickness * cellX * cellZ;
	const double centerX = (i + 0.5) * cellX;
	const double centerY = base + 0.5 * thickness;
	const double depth = verticalReclaim || horizontalReclaim ? 0.0 : thickness / std::abs(tanReclaimAngle);
	if (this->simulationParameters.circular) {
		const double dx = centerX - 0.5 * this->simulationParameters.heapWorldSizeX;
		const double dz = (j + 0.5) * cellZ - 0.5 * this->simulationParameters.heapWorldSizeZ;
		double position = 0.5 * (1.0 - std::atan2(dz, dx) / this->pi) * circumference;
		// Horizontal reclaiming is ignored for circular stockpiles, like in the fast simulation
		if (!verticalReclaim && !horizontalReclaim) {
			position -= centerY / tanReclaimAngle;
		}
		slices.push(position, cellX, depth, parameters, share, true);
	} else if (horizontalReclaim) {
		slices.push(this->simulationParameters.reclaimAngle < 90.0f ? 0 : int(slices.size()) - 1, parameters, share);
	} else {
		slices.push(verticalReclaim ? centerX : centerX - centerY / tanReclaimAngle, cellX, depth, parameters, share);
	}
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorCone<Parameters>::updateHeapMap()
{
	for (int j = 0; j < nz; j++) {
		for (int i = 0; i < nx; i++) {
			this->heapMap[std::size_t(j) * nx + i] = float(heights[std::size_t(i) * nz + j]);
		}
	}
}
