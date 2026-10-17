#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <thread>

#include "BlendingSimulator/Particle.h"

template<typename Parameters>
blendingsimulator::BlendingSimulatorDetailed<Parameters>::BlendingSimulatorDetailed(SimulationParameters simulationParameters)
	: BlendingSimulator<Parameters>(simulationParameters)
{
	const DetailedParameters& d = simulationParameters.detailed;
	if (simulationParameters.particlesPerCubicMeter <= 0.0f || d.packingFraction <= 0.0 || d.packingFraction > 1.0 || d.sizeSpread < 0.0 ||
		d.sizeSpread >= 1.0 || d.flowRate <= 0.0) {
		throw std::invalid_argument("invalid parameters of the detailed simulation");
	}
	particleVolume = 1.0 / simulationParameters.particlesPerCubicMeter;
	// Radii are uniform in r0 (1 +/- s), and the mean of (1 + u)^3 for u uniform in +/- s is 1 + s^2: the mean solid volume of a particle
	// is packingFraction times its bulk volume
	const double s = d.sizeSpread;
	meanRadius = std::cbrt(3.0 * d.packingFraction * particleVolume / (4.0 * this->pi * (1.0 + s * s)));
	cellSize = std::cbrt(particleVolume);

	// The stream enters the simulation in a horizontal plane a little below the head pulley, moving down at the speed of the drop so far and
	// along +z at the belt speed. Each particle is released at a free point of a lattice in that plane, spaced so that particles of the
	// largest size do not touch, with enough points (twice as many as needed) that each is free again when particles are next due there:
	// its last particle has fallen or moved one spacing away. Rows are staggered by half a point, so that a particle moving along z with
	// the belt passes between the points of the next row. The stream thus delivers the flow at any particle size, and its extent
	// hardly depends on the particle size (about 1 m along z at the defaults, over the belt width in x)
	constexpr double g = 9.80665;
	const double maxRadius = meanRadius * (1.0 + s);
	streamSpacing = 2.04 * maxRadius;
	streamColumns = std::max(1, int(d.beltWidth / streamSpacing));
	streamCellWidth = std::max(d.beltWidth / streamColumns, streamSpacing);
	streamDrop = std::min(0.25, 0.5 * d.dischargeHeight);
	streamDropSpeed = std::sqrt(2.0 * g * streamDrop);
	streamClearTime = (std::sqrt(streamDropSpeed * streamDropSpeed + 2.0 * g * streamSpacing) - streamDropSpeed) / g;
	if (d.beltSpeed > 0.0) {
		streamClearTime = std::min(streamClearTime, streamSpacing / d.beltSpeed);
	}
	const double points = std::ceil(2.0 * streamClearTime * d.flowRate / particleVolume);
	streamRows = int(std::min(std::max(1.0, std::ceil(points / streamColumns)), 1e5));

	this->initializeHeapMap(
		std::max(1u, (unsigned int)(simulationParameters.heapWorldSizeX / cellSize + 0.5)),
		std::max(1u, (unsigned int)(simulationParameters.heapWorldSizeZ / cellSize + 0.5))
	);
	createWorld();
}

template<typename Parameters>
blendingsimulator::BlendingSimulatorDetailed<Parameters>::~BlendingSimulatorDetailed()
{
	deleteOutputParticles();
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorDetailed<Parameters>::createWorld()
{
	const SimulationParameters& p = this->simulationParameters;
	const DetailedParameters& d = p.detailed;

	dem::Material material;
	material.density = d.density;
	material.youngsModulus = d.youngsModulus;
	material.poissonRatio = d.poissonRatio;
	material.restitution = d.restitution;
	material.friction = d.friction;
	material.rollingFriction = d.rollingFriction;
	material.rollingDamping = d.rollingDamping;
	material.groundFriction = d.friction;
	material.groundRollingFriction = d.rollingFriction;

	// Material that leaves the bed by more than the margin is lost; the pile of a full bed reaches its edges, its toe a little beyond
	const double margin = std::max(5.0, 0.5 * double(p.heapWorldSizeZ));
	dem::WorldSettings settings;
	settings.minX = -margin;
	settings.maxX = p.heapWorldSizeX + margin;
	settings.minZ = -margin;
	settings.maxZ = p.heapWorldSizeZ + margin;
	settings.timeStepFactor = d.timeStepFactor;
	settings.skinFactor = d.skinFactor;
	settings.restVelocity = d.restVelocity;
	settings.restTime = d.restTime;
	settings.wakeVelocity = 2.0 * d.restVelocity;
	settings.threads = d.threads;

	world = std::make_unique<dem::World>(settings, material, meanRadius * (1.0 - d.sizeSpread), meanRadius * (1.0 + d.sizeSpread));
	particleParameters.clear();
	releaseTime = 0.0;
	blockedTime = 0.0;
	nextOutput = 0.0;
	reclaimPrepared = false;
	slices = ReclaimSlices<Parameters>();
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorDetailed<Parameters>::clear()
{
	deleteOutputParticles();
	createWorld();
	std::fill(this->heapMap, this->heapMap + std::size_t(this->heapSizeX) * this->heapSizeZ, 0.0f);
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorDetailed<Parameters>::advanceTo(double t)
{
	while (world->time() < t) {
		while (this->paused.load()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		world->step();
		if (this->simulationParameters.visualize && world->time() >= nextOutput) {
			updateOutputParticles(false);
			nextOutput = world->time() + outputInterval;
		}
	}
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorDetailed<Parameters>::stackSingle(float x, float z, const Parameters& parameters)
{
	if (reclaimPrepared) {
		throw std::logic_error("cannot stack after reclaiming started");
	}
	const DetailedParameters& d = this->simulationParameters.detailed;

	// Draw in a fixed order so that a seed reproduces the same particles
	auto& generator = this->randomEngine;
	std::uniform_real_distribution<double> unit(-1.0, 1.0);
	const double radius = meanRadius * (1.0 + d.sizeSpread * unit(generator));
	const double across = unit(generator);
	const double along = unit(generator);
	const double speed = d.beltSpeed * (1.0 + 0.05 * unit(generator));
	const std::uint64_t points = std::uint64_t(streamColumns) * std::uint64_t(streamRows);
	const std::uint64_t firstPoint = generator() % points;

	// The particle leaves the boom when its volume has been conveyed; after a pause of the stacker (finishStacking while stacking
	// continues later) the conveying goes on from the current time. The world reaches a release time up to a time step late, which must
	// not add up
	if (world->time() > releaseTime + world->timeStep()) {
		releaseTime = world->time();
	}
	releaseTime += particleVolume / d.flowRate;
	advanceTo(releaseTime);

	// The release plane lies dischargeHeight - streamDrop above the pile at the stacker position (the boom luffs with the pile). The stream
	// is aimed so that its center lands at the stacker position: falling from the plane takes t with h = v t + g t^2 / 2, during which the
	// material travels speed * t along z. The particle takes the first free release point from a random one on; if all are taken (the
	// pile reached the discharge), it waits on the belt
	constexpr double g = 9.80665;
	const double height = std::max(0.0, d.dischargeHeight - streamDrop);
	const double fallTime = (std::sqrt(streamDropSpeed * streamDropSpeed + 2.0 * g * height) - streamDropSpeed) / g;
	dem::Vec3 start;
	for (int wait = 0;; wait++) {
		const double surface = world->surfaceHeight(x, z, 2.0 * meanRadius);
		bool free = false;
		for (std::uint64_t k = 0; k < points && !free; k++) {
			const std::uint64_t point = (firstPoint + k) % points;
			const std::uint64_t rowIndex = point / std::uint64_t(streamColumns);
			const double column = double(point % std::uint64_t(streamColumns)) - 0.5 * (streamColumns - 1) + (rowIndex % 2 ? 0.25 : -0.25);
			const double row = double(rowIndex) - 0.5 * (streamRows - 1);
			start = dem::Vec3(
				x + column * streamCellWidth + 0.5 * (streamCellWidth - 2.0 * radius) * across,
				surface + height + radius,
				z - speed * fallTime + row * streamSpacing + 0.5 * (streamSpacing - 2.0 * radius) * along
			);
			free = !world->overlaps(start, radius);
		}
		if (free) {
			break;
		}
		if (wait > 10000) {
			throw std::runtime_error("the stacker's discharge stays blocked");
		}
		const double blockedSince = world->time();
		advanceTo(world->time() + 0.25 * streamClearTime);
		blockedTime += world->time() - blockedSince;
		releaseTime = world->time();
	}
	const int id = world->addParticle(start, dem::Vec3(0.0, -streamDropSpeed, speed), radius);
	if (id != int(particleParameters.size())) {
		throw std::logic_error("particle indices out of step");
	}
	particleParameters.push_back(parameters);
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorDetailed<Parameters>::finishStacking()
{
	if (reclaimPrepared) {
		return;
	}
	const double deadline = world->time() + maxSettleTime;
	while (!world->settled() && world->time() < deadline) {
		advanceTo(world->time() + 0.1);
	}
	if (this->simulationParameters.visualize) {
		updateOutputParticles(true);
	}
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorDetailed<Parameters>::prepareReclaim()
{
	finishStacking();
	const SimulationParameters& p = this->simulationParameters;
	const double width = cellSize;

	double tanReclaimAngle;
	if (std::abs(90.0f - p.reclaimAngle) < 0.01) {
		tanReclaimAngle = 1e100;
	} else {
		tanReclaimAngle = std::tan(p.reclaimAngle * std::atan(1.0) * 4.0 / 180.0);
	}
	const bool vertical = tanReclaimAngle > 1e10;
	const bool horizontal = tanReclaimAngle < 1e-10;

	double circumference = 0.0;
	if (p.circular) {
		circumference = 2.0 * this->pi * 0.25 * std::min(p.heapWorldSizeX, p.heapWorldSizeZ);
		slices.resize(std::max(1u, (unsigned int)(circumference / width + 0.5)), float(width));
	} else {
		slices.resize(std::max(1u, (unsigned int)(p.heapWorldSizeX / width + 0.5)), float(width));
	}
	slices.clear();

	for (std::size_t i = 0; i < particleParameters.size(); i++) {
		if (world->getState(int(i)) == dem::World::State::Removed) {
			continue;
		}
		const dem::Vec3& c = world->getPosition(int(i));
		const double size = 2.0 * world->getRadius(int(i));
		const double depth = vertical || horizontal ? 0.0 : size / std::abs(tanReclaimAngle);
		if (p.circular) {
			const double dx = c.x - 0.5 * p.heapWorldSizeX;
			const double dz = c.z - 0.5 * p.heapWorldSizeZ;
			double along = 0.5 * (1.0 - std::atan2(dz, dx) / this->pi) * circumference;
			if (!vertical && !horizontal) {
				along -= c.y / tanReclaimAngle;
			}
			slices.push(along, size, depth, particleParameters[i], true);
		} else if (horizontal) {
			slices.push(p.reclaimAngle < 90.0f ? 0 : int(slices.size()) - 1, particleParameters[i]);
		} else {
			slices.push(vertical ? c.x : c.x - c.y / tanReclaimAngle, size, depth, particleParameters[i]);
		}
	}
	reclaimPrepared = true;
}

template<typename Parameters>
bool blendingsimulator::BlendingSimulatorDetailed<Parameters>::reclaimingFinished()
{
	if (!reclaimPrepared) {
		prepareReclaim();
	}
	return slices.finished();
}

template<typename Parameters>
Parameters blendingsimulator::BlendingSimulatorDetailed<Parameters>::reclaim(float position)
{
	if (!reclaimPrepared) {
		prepareReclaim();
	}
	return slices.reclaim(position);
}

template<typename Parameters>
double blendingsimulator::BlendingSimulatorDetailed<Parameters>::getLostVolume() const
{
	double lost = 0.0;
	for (std::size_t i = 0; i < particleParameters.size(); i++) {
		if (world->getState(int(i)) == dem::World::State::Removed) {
			lost += particleVolume;
		}
	}
	return lost;
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorDetailed<Parameters>::updateHeapMap()
{
	// Top surface of the spheres: every particle raises the cells under it to the height of its cap
	const unsigned int nx = this->heapSizeX;
	const unsigned int nz = this->heapSizeZ;
	const double sx = this->simulationParameters.heapWorldSizeX / nx;
	const double sz = this->simulationParameters.heapWorldSizeZ / nz;
	std::fill(this->heapMap, this->heapMap + std::size_t(nx) * nz, 0.0f);
	for (std::size_t i = 0; i < world->particleCount(); i++) {
		if (world->getState(int(i)) == dem::World::State::Removed) {
			continue;
		}
		const dem::Vec3& c = world->getPosition(int(i));
		const double r = world->getRadius(int(i));
		const int x0 = std::max(0, int(std::floor((c.x - r) / sx)));
		const int x1 = std::min(int(nx) - 1, int(std::floor((c.x + r) / sx)));
		const int z0 = std::max(0, int(std::floor((c.z - r) / sz)));
		const int z1 = std::min(int(nz) - 1, int(std::floor((c.z + r) / sz)));
		for (int zi = z0; zi <= z1; zi++) {
			for (int xi = x0; xi <= x1; xi++) {
				const double dx = (xi + 0.5) * sx - c.x;
				const double dz = (zi + 0.5) * sz - c.z;
				const double d2 = r * r - dx * dx - dz * dz;
				if (d2 < 0.0) {
					continue;
				}
				float& h = this->heapMap[std::size_t(zi) * nx + xi];
				h = std::max(h, float(c.y + std::sqrt(d2)));
			}
		}
	}
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorDetailed<Parameters>::updateOutputParticles(bool final)
{
	std::lock_guard<std::mutex> lock(this->outputParticlesMutex);
	this->activeOutputParticles.clear();
	this->inactiveOutputParticles.clear();
	for (std::size_t i = 0; i < world->particleCount(); i++) {
		if (outputParticles.size() <= i) {
			auto* particle = new Particle<Parameters>();
			particle->parameters = particleParameters[i];
			particle->orientation = Quaternion(1, 0, 0, 0);
			outputParticles.push_back(particle);
		}
		const dem::World::State state = world->getState(int(i));
		if (state == dem::World::State::Removed) {
			continue;
		}
		Particle<Parameters>* particle = outputParticles[i];
		const dem::Vec3& c = world->getPosition(int(i));
		const double size = 2.0 * world->getRadius(int(i));
		particle->position = Vector3(c.x, c.y, c.z);
		particle->size = Vector3(size, size, size);
		particle->frozen = final || state == dem::World::State::Fixed;
		(particle->frozen ? this->inactiveOutputParticles : this->activeOutputParticles).push_back(particle);
	}
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorDetailed<Parameters>::deleteOutputParticles()
{
	std::lock_guard<std::mutex> lock(this->outputParticlesMutex);
	this->activeOutputParticles.clear();
	this->inactiveOutputParticles.clear();
	for (auto* particle : outputParticles) {
		delete particle;
	}
	outputParticles.clear();
}
