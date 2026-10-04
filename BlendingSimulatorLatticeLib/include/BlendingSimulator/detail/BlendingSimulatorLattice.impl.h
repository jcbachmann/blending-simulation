#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <thread>

#include "BlendingSimulator/Particle.h"

template<typename Parameters>
blendingsimulator::BlendingSimulatorLattice<Parameters>::BlendingSimulatorLattice(SimulationParameters simulationParameters)
	: BlendingSimulator<Parameters>(simulationParameters)
{
	if (simulationParameters.circular) {
		throw std::invalid_argument("the lattice simulation does not support circular stockpiles");
	}

	const double degrees = std::atan(1.0) * 4.0 / 180.0;
	verticalScale = std::tan(simulationParameters.latticeAngleOfRepose * degrees) / std::tan(nativeAngleOfRepose * degrees);

	// A close-packed sphere of diameter d occupies d^3 / sqrt(2), compressed by the vertical scale; that is the particle volume
	const double particleVolume = 1.0 / simulationParameters.particlesPerCubicMeter;
	diameter = std::cbrt(std::sqrt(2.0) * particleVolume / verticalScale);
	rowDistance = diameter * std::sqrt(3.0) / 2.0;
	layerDistance = diameter * std::sqrt(2.0 / 3.0) * verticalScale;

	if (std::abs(90.0f - simulationParameters.reclaimAngle) < 0.01) {
		tanReclaimAngle = 1e100;
	} else {
		tanReclaimAngle = std::tan(simulationParameters.reclaimAngle * degrees);
	}

	// Columns whose sites can lie on the bed, with a margin
	minX = -2;
	minZ = -2;
	columnsX = static_cast<int>(std::ceil(simulationParameters.heapWorldSizeX / diameter)) + 4;
	columnsZ = static_cast<int>(std::ceil(simulationParameters.heapWorldSizeZ / rowDistance)) + 4;
	heights.assign(static_cast<size_t>(columnsX) * columnsZ, 0);

	for (int p = 0; p < 2; p++) {
		valid[p].assign(heights.size(), 0);
		for (int zi = minZ; zi < minZ + columnsZ; zi++) {
			for (int xi = minX; xi < minX + columnsX; xi++) {
				double xu;
				double zu;
				toUnits({xi, p, zi}, xu, zu);
				const double x = (xu + 0.5) * diameter;
				const double z = zu * rowDistance + 0.5 * diameter;
				valid[p][column(xi, zi)] =
					x >= 0.0 && x < simulationParameters.heapWorldSizeX && z >= 0.0 && z < simulationParameters.heapWorldSizeZ;
			}
		}
	}

	this->initializeHeapMap(
		(unsigned int)(simulationParameters.heapWorldSizeX / diameter + 0.5),
		(unsigned int)(simulationParameters.heapWorldSizeZ / diameter + 0.5)
	);
	slices.resize(this->heapSizeX, static_cast<float>(diameter));

	clear();
}

template<typename Parameters>
double blendingsimulator::BlendingSimulatorLattice<Parameters>::getParticleDiameter() const
{
	return diameter;
}

template<typename Parameters>
double blendingsimulator::BlendingSimulatorLattice<Parameters>::getParticleHeight() const
{
	return diameter * verticalScale;
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorLattice<Parameters>::clear()
{
	std::fill(heights.begin(), heights.end(), 0);
	maxHeight = 0;
	slices.clear();

	std::lock_guard<std::mutex> lock(this->outputParticlesMutex);
	for (auto particle : this->activeOutputParticles) {
		delete particle;
	}
	this->activeOutputParticles.clear();
	for (auto particle : this->inactiveOutputParticles) {
		delete particle;
	}
	this->inactiveOutputParticles.clear();
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorLattice<Parameters>::finishStacking()
{
	// Nothing to do
}

template<typename Parameters>
bool blendingsimulator::BlendingSimulatorLattice<Parameters>::reclaimingFinished()
{
	return slices.finished();
}

template<typename Parameters>
Parameters blendingsimulator::BlendingSimulatorLattice<Parameters>::reclaim(float position)
{
	return slices.reclaim(position);
}

template<typename Parameters>
int blendingsimulator::BlendingSimulatorLattice<Parameters>::parity(int i)
{
	return ((i % 2) + 2) % 2;
}

template<typename Parameters>
int blendingsimulator::BlendingSimulatorLattice<Parameters>::column(int xi, int zi) const
{
	const int x = xi - minX;
	const int z = zi - minZ;
	if (x < 0 || x >= columnsX || z < 0 || z >= columnsZ) {
		return -1;
	}
	return z * columnsX + x;
}

template<typename Parameters>
bool blendingsimulator::BlendingSimulatorLattice<Parameters>::isValid(int xi, int yi, int zi) const
{
	if (yi < 0) {
		return false;
	}
	const int c = column(xi, zi);
	return c >= 0 && valid[parity(yi)][c];
}

template<typename Parameters>
bool blendingsimulator::BlendingSimulatorLattice<Parameters>::isFilled(int xi, int yi, int zi) const
{
	// Sites outside of the bed act like walls
	if (!isValid(xi, yi, zi)) {
		return true;
	}
	return heights[column(xi, zi)] > yi;
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorLattice<Parameters>::spacesBelow(const Site& site, Site below[3]) const
{
	const int ys = parity(site.yi);
	const int zs = parity(site.zi);
	below[0] = {site.xi + ys + zs - ys * zs - 1, site.yi - 1, site.zi + ys - 1};
	below[1] = {site.xi + ys, site.yi - 1, site.zi};
	below[2] = {site.xi + ys + zs * ys - 1, site.yi - 1, site.zi + ys};
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorLattice<Parameters>::toUnits(const Site& site, double& xu, double& zu) const
{
	xu = site.xi + parity(site.zi) / 2.0 + parity(site.yi) / 2.0;
	zu = site.zi + parity(site.yi) / 3.0;
}

template<typename Parameters>
typename blendingsimulator::BlendingSimulatorLattice<Parameters>::Site
blendingsimulator::BlendingSimulatorLattice<Parameters>::fromUnits(double xu, double zu, int yi) const
{
	// std::nearbyint rounds halves to even like Python's round(), which the reference implementation uses
	const int p = parity(yi);
	const int zi = static_cast<int>(std::nearbyint(zu - p / 3.0));
	const int xi = static_cast<int>(std::nearbyint(xu - p / 2.0 - parity(zi) / 2.0));
	return {xi, yi, zi};
}

template<typename Parameters>
bool blendingsimulator::BlendingSimulatorLattice<Parameters>::drop(double xu, double zu, Site& site) const
{
	// The particle comes to rest on the first site from above that is free and touches the pile, that is has a filled or invalid site
	// below. Instead of searching from the top of the pile, use that the drop column depends only on the layer parity: a site of parity p
	// touches the pile exactly up to the highest of the columns below it (the drop column is one of them), or at any height if one of
	// them lies outside of the bed. The highest touching site of either parity is the first one the search from above would meet.
	int top = -1;
	for (int p = 0; p < 2; p++) {
		const Site candidate = fromUnits(xu, zu, p);
		if (!isValid(candidate.xi, p, candidate.zi)) {
			continue;
		}
		Site below[3];
		spacesBelow(candidate, below);
		int supportTop = 0;
		bool wall = false;
		for (const Site& s : below) {
			const int c = column(s.xi, s.zi);
			if (c < 0 || !valid[1 - p][c]) {
				wall = true;
			} else {
				supportTop = std::max(supportTop, heights[c]);
			}
		}
		int level = wall ? maxHeight : std::min(supportTop, maxHeight);
		if (parity(level) != p) {
			level--;
		}
		if (level >= 0) {
			top = std::max(top, level);
		}
	}
	if (top < 0) {
		return false;
	}

	site = fromUnits(xu, zu, top);
	if (isFilled(site.xi, site.yi, site.zi)) {
		return false;
	}

	// Fall until all three sites below are filled, preferring the site closest to the drop position
	while (true) {
		Site below[3];
		spacesBelow(site, below);
		int freeCount = 0;
		Site best{};
		double bestDistance = 0.0;
		for (const Site& s : below) {
			if (isFilled(s.xi, s.yi, s.zi)) {
				continue;
			}
			double sxu;
			double szu;
			toUnits(s, sxu, szu);
			const double distance = std::hypot((xu - sxu) * diameter, (zu - szu) * rowDistance);
			if (freeCount == 0 || distance < bestDistance) {
				best = s;
				bestDistance = distance;
			}
			freeCount++;
		}
		if (freeCount == 0) {
			return true;
		}
		site = best;
	}
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorLattice<Parameters>::stackSingle(float x, float z, const Parameters& parameters)
{
	while (this->paused.load()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	const double xu = x / diameter - 0.5;
	const double zu = (z - 0.5 * diameter) / rowDistance;
	Site site{};
	if (!drop(xu, zu, site)) {
		// The particle does not fit on the bed
		return;
	}

	heights[column(site.xi, site.zi)] = site.yi + 1;
	maxHeight = std::max(maxHeight, site.yi + 1);

	double sxu;
	double szu;
	toUnits(site, sxu, szu);
	const double worldX = (sxu + 0.5) * diameter;
	const double worldZ = szu * rowDistance + 0.5 * diameter;
	const double base = site.yi * layerDistance;

	if (this->simulationParameters.visualize) {
		auto particle = new Particle<Parameters>();
		particle->parameters = parameters;
		particle->frozen = true;
		particle->position = Vector3(worldX, base + 0.5 * getParticleHeight(), worldZ);
		particle->size = Vector3(diameter, getParticleHeight(), diameter);
		particle->orientation = Quaternion(1, 0, 0, 0);
		std::lock_guard<std::mutex> lock(this->outputParticlesMutex);
		this->inactiveOutputParticles.push_back(particle);
	}

	// A particle with its base at height y above ground position x is reclaimed at x - y / tan(reclaim angle)
	int reclaimIndex;
	const int last = static_cast<int>(slices.size()) - 1;
	if (tanReclaimAngle > 1e10) {
		reclaimIndex = static_cast<int>(std::floor(worldX / diameter));
	} else if (tanReclaimAngle < 1e-10) {
		reclaimIndex = this->simulationParameters.reclaimAngle < 90.0f ? 0 : last;
	} else {
		reclaimIndex = static_cast<int>(std::floor((worldX - base / tanReclaimAngle) / diameter));
	}
	slices.push(std::max(0, std::min(reclaimIndex, last)), parameters);
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorLattice<Parameters>::updateHeapMap()
{
	std::fill(this->heapMap, this->heapMap + this->heapSizeX * this->heapSizeZ, 0.0f);
	for (int zi = minZ; zi < minZ + columnsZ; zi++) {
		for (int xi = minX; xi < minX + columnsX; xi++) {
			const int h = heights[column(xi, zi)];
			if (h <= 0) {
				continue;
			}
			// The top site of the column, whose layer reaches h layers high
			double xu;
			double zu;
			toUnits({xi, h - 1, zi}, xu, zu);
			const int cx = static_cast<int>(std::floor(xu + 0.5));
			const int cz = static_cast<int>(std::floor((zu * rowDistance + 0.5 * diameter) / diameter));
			if (cx < 0 || cx >= static_cast<int>(this->heapSizeX) || cz < 0 || cz >= static_cast<int>(this->heapSizeZ)) {
				continue;
			}
			float& cell = this->heapMap[cz * this->heapSizeX + cx];
			cell = std::max(cell, static_cast<float>(h * layerDistance));
		}
	}
}
