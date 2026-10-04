#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "BlendingSimulator/BlendingSimulatorLattice.h"
#include "BlendingSimulator/ParticleParameters.h"

namespace bs = blendingsimulator;

bs::SimulationParameters latticeParameters(float size, float particlesPerCubicMeter, float angle = 45.0f)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = size;
	simulationParameters.heapWorldSizeZ = size;
	simulationParameters.reclaimAngle = 45.0f;
	simulationParameters.particlesPerCubicMeter = particlesPerCubicMeter;
	simulationParameters.latticeAngleOfRepose = angle;
	return simulationParameters;
}

using Lattice = bs::BlendingSimulatorLattice<bs::AveragedParameters>;

const float nativeAngle = static_cast<float>(Lattice::nativeAngleOfRepose());

double scaleFor(double angle)
{
	const double degrees = std::atan(1.0) * 4.0 / 180.0;
	return std::tan(angle * degrees) / Lattice::nativeTanAngleOfRepose();
}

double toDegrees(double radians)
{
	return radians * 45.0 / std::atan(1.0);
}

// Slope of a straight line fitted to the points (distance, height) between 20 % and 80 % of the peak height
double fitSlope(const std::vector<std::pair<double, double>>& points, double peak)
{
	double n = 0.0, sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
	for (const auto& [x, y] : points) {
		if (y > 0.2 * peak && y < 0.8 * peak) {
			n += 1.0;
			sx += x;
			sy += y;
			sxx += x * x;
			sxy += x * y;
		}
	}
	return (n * sxy - sx * sy) / (n * sxx - sx * sx);
}

// A pile of the given volume stacked at the center of a 20 m by 20 m bed with fine particles
std::unique_ptr<Lattice> centerPile(float angle, double volume)
{
	auto simulator = std::make_unique<Lattice>(latticeParameters(20.0f, 512.0f, angle));
	simulator->stack(10.0f, 10.0f, bs::AveragedParameters(volume, {1.0}));
	return simulator;
}

TEST(BlendingSimulatorLattice, test_particle_volume)
{
	for (float angle : {30.0f, 45.0f, nativeAngle}) {
		bs::BlendingSimulatorLattice<bs::AveragedParameters> simulator(latticeParameters(10.0f, 8.0f, angle));
		const double d = simulator.getParticleDiameter();
		const double h = simulator.getParticleHeight();
		// Close-packed spheres occupy d^3 / sqrt(2), compressed by the vertical scale h / d
		EXPECT_NEAR(d * d * d * (h / d) / std::sqrt(2.0), 1.0 / 8.0, 1e-9);
		EXPECT_NEAR(h / d, scaleFor(angle), 1e-9);
	}
}

TEST(BlendingSimulatorLattice, test_single_particle)
{
	bs::SimulationParameters simulationParameters = latticeParameters(10.0f, 8.0f);
	simulationParameters.visualize = true;
	bs::BlendingSimulatorLattice<bs::AveragedParameters> simulator(simulationParameters);

	simulator.stack(5.0f, 5.0f, bs::AveragedParameters(0.125, {1.0}));

	ASSERT_EQ(simulator.inactiveOutputParticles.size(), 1);
	const auto* particle = simulator.inactiveOutputParticles.front();
	EXPECT_NEAR(particle->position.y, 0.5 * simulator.getParticleHeight(), 1e-9);
	EXPECT_LT(std::hypot(particle->position.x - 5.0, particle->position.z - 5.0), simulator.getParticleDiameter());
}

TEST(BlendingSimulatorLattice, test_no_overlap)
{
	// Uncompressed lattice, so that touching spheres have one diameter between their centers
	bs::SimulationParameters simulationParameters = latticeParameters(10.0f, 8.0f, nativeAngle);
	simulationParameters.visualize = true;
	bs::BlendingSimulatorLattice<bs::AveragedParameters> simulator(simulationParameters);

	simulator.stack(5.0f, 5.0f, bs::AveragedParameters(40.0, {1.0}));

	std::vector<bs::Vector3> positions;
	for (const auto* particle : simulator.inactiveOutputParticles) {
		positions.push_back(particle->position);
	}
	ASSERT_EQ(positions.size(), 320);
	double minimum = 1e100;
	for (size_t i = 0; i < positions.size(); i++) {
		for (size_t j = i + 1; j < positions.size(); j++) {
			const double dx = positions[i].x - positions[j].x;
			const double dy = positions[i].y - positions[j].y;
			const double dz = positions[i].z - positions[j].z;
			minimum = std::min(minimum, std::sqrt(dx * dx + dy * dy + dz * dz));
		}
	}
	EXPECT_NEAR(minimum, simulator.getParticleDiameter(), 1e-6);
	// The pile grows above the first layer
	double top = 0.0;
	for (const auto& p : positions) {
		top = std::max(top, p.y);
	}
	EXPECT_GT(top, 2.0 * simulator.getParticleHeight());
}

TEST(BlendingSimulatorLattice, test_reclaim_volume)
{
	bs::BlendingSimulatorLattice<bs::AveragedParameters> simulator(latticeParameters(20.0f, 8.0f));
	for (int i = 0; i < 100; i++) {
		simulator.stack(5.0f + 0.1f * i, 10.0f, bs::AveragedParameters(1.0, {i % 2 == 0 ? 1.0 : 3.0}));
	}
	simulator.finishStacking();

	double volume = 0.0;
	double weighted = 0.0;
	float position = 0.0f;
	while (!simulator.reclaimingFinished()) {
		position += 1.0f;
		const bs::AveragedParameters p = simulator.reclaim(position);
		volume += p.getVolume();
		weighted += p.getVolume() * p.getValue(0);
	}
	EXPECT_NEAR(volume, 100.0, 1e-6);
	EXPECT_NEAR(weighted / volume, 2.0, 1e-6);
}

TEST(BlendingSimulatorLattice, test_compression_scales_heights)
{
	// Same diameter: the uncompressed particles hold the volume of the compressed ones divided by the scale
	const double scale = scaleFor(45.0);
	bs::BlendingSimulatorLattice<bs::AveragedParameters> compressed(latticeParameters(10.0f, 8.0f, 45.0f));
	bs::BlendingSimulatorLattice<bs::AveragedParameters> native(latticeParameters(10.0f, static_cast<float>(8.0 * scale), nativeAngle));
	ASSERT_NEAR(compressed.getParticleDiameter(), native.getParticleDiameter(), 1e-6);

	// A drop position off the symmetry points of the lattice, where equally distant sites would be chosen by rounding differences, and a
	// little more than one particle per call, so that the float particle volume of the native lattice cannot hold back a particle
	for (int i = 0; i < 400; i++) {
		compressed.stack(5.137f, 4.921f, bs::AveragedParameters(1.000001 / 8.0, {1.0}));
		native.stack(5.137f, 4.921f, bs::AveragedParameters(1.000001 / (8.0 * scale), {1.0}));
	}

	auto size = compressed.getHeapMapSize();
	ASSERT_EQ(size, native.getHeapMapSize());
	const float* c = compressed.getHeapMap();
	const float* n = native.getHeapMap();
	double peak = 0.0;
	for (unsigned int i = 0; i < size.first * size.second; i++) {
		EXPECT_NEAR(c[i], n[i] * scale, 1e-3);
		peak = std::max(peak, double(n[i]));
	}
	EXPECT_GT(peak, 1.0);
}

TEST(BlendingSimulatorLattice, test_heap_map_without_holes)
{
	bs::BlendingSimulatorLattice<bs::AveragedParameters> simulator(latticeParameters(20.0f, 1.0f));
	for (int i = 0; i < 300; i++) {
		simulator.stack(10.0f, 10.0f, bs::AveragedParameters(1.0, {1.0}));
	}

	// A cone of 300 m³ at 45° has a radius of about 6.6 m: every cell within 4 m of its axis lies on the pile, rising towards the axis
	auto size = simulator.getHeapMapSize();
	const float* heights = simulator.getHeapMap();
	const double d = simulator.getParticleDiameter();
	int cells = 0;
	for (unsigned int z = 0; z < size.second; z++) {
		for (unsigned int x = 0; x < size.first; x++) {
			const double r = std::hypot((x + 0.5) * d - 10.0, (z + 0.5) * d - 10.0);
			if (r < 4.0) {
				EXPECT_GT(heights[z * size.first + x], 6.6 - r - 2.0 * d) << "cell " << x << ", " << z;
				cells++;
			}
		}
	}
	EXPECT_GT(cells, 20);
}

TEST(BlendingSimulatorLattice, test_native_angle_of_repose)
{
	EXPECT_NEAR(toDegrees(std::atan(Lattice::nativeTanAngleOfRepose())), 60.887, 1e-3);
	EXPECT_NEAR(Lattice::nativeAngleOfRepose(), 60.887, 1e-3);
}

TEST(BlendingSimulatorLattice, test_native_pile_is_hexagonal_pyramid)
{
	const auto simulator = centerPile(nativeAngle, 300.0);
	auto size = simulator->getHeapMapSize();
	const float* heights = simulator->getHeapMap();
	const double d = simulator->getParticleDiameter();
	const unsigned int cx = size.first / 2;
	const unsigned int cz = size.second / 2;

	// Along x the pile falls along its edges, along z (across the lattice rows) down its faces
	std::vector<std::pair<double, double>> alongX;
	std::vector<std::pair<double, double>> alongZ;
	double peak = 0.0;
	for (unsigned int x = 0; x < size.first; x++) {
		alongX.emplace_back(std::abs((x + 0.5) * d - 10.0), heights[cz * size.first + x]);
		peak = std::max(peak, double(heights[cz * size.first + x]));
	}
	for (unsigned int z = 0; z < size.second; z++) {
		alongZ.emplace_back(std::abs((z + 0.5) * d - 10.0), heights[z * size.first + cx]);
	}
	EXPECT_NEAR(toDegrees(std::atan(-fitSlope(alongX, peak))), toDegrees(std::atan(2.0 * std::sqrt(2.0 / 3.0))), 1.0);
	EXPECT_NEAR(toDegrees(std::atan(-fitSlope(alongZ, peak))), toDegrees(std::atan(4.0 * std::sqrt(2.0) / 3.0)), 1.0);
}

TEST(BlendingSimulatorLattice, test_pile_matches_cone_of_angle_of_repose)
{
	for (float angle : {30.0f, 45.0f}) {
		const auto simulator = centerPile(angle, 300.0);
		auto size = simulator->getHeapMapSize();
		const float* heights = simulator->getHeapMap();
		const double d = simulator->getParticleDiameter();
		double peak = 0.0;
		for (unsigned int i = 0; i < size.first * size.second; i++) {
			peak = std::max(peak, double(heights[i]));
		}
		// The radius of the circle with the area of the pile above each height falls with the slope of the equivalent cone
		std::vector<std::pair<double, double>> radii;
		for (int k = 1; k < 50; k++) {
			const double level = peak * k / 50.0;
			int cells = 0;
			for (unsigned int i = 0; i < size.first * size.second; i++) {
				cells += heights[i] > level;
			}
			radii.emplace_back(std::sqrt(cells * d * d / (std::atan(1.0) * 4.0)), level);
		}
		EXPECT_NEAR(toDegrees(std::atan(-fitSlope(radii, peak))), angle, 1.0) << "angle " << angle;
	}
}

TEST(BlendingSimulatorLattice, test_clear)
{
	bs::BlendingSimulatorLattice<bs::AveragedParameters> simulator(latticeParameters(10.0f, 1.0f));
	simulator.stack(5.0f, 5.0f, bs::AveragedParameters(20.0, {1.0}));
	simulator.clear();
	auto size = simulator.getHeapMapSize();
	const float* heights = simulator.getHeapMap();
	for (unsigned int i = 0; i < size.first * size.second; i++) {
		EXPECT_EQ(heights[i], 0.0f);
	}
}

TEST(BlendingSimulatorLattice, test_circular_not_supported)
{
	bs::SimulationParameters simulationParameters = latticeParameters(10.0f, 1.0f);
	simulationParameters.circular = true;
	EXPECT_THROW(bs::BlendingSimulatorLattice<bs::AveragedParameters> simulator(simulationParameters), std::invalid_argument);
}
