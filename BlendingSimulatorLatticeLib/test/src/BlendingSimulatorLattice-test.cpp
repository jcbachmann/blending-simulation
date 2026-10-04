#include <gtest/gtest.h>

#include <cmath>
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

double scaleFor(double angle)
{
	const double degrees = std::atan(1.0) * 4.0 / 180.0;
	return std::tan(angle * degrees) / std::tan(bs::BlendingSimulatorLattice<bs::AveragedParameters>::nativeAngleOfRepose * degrees);
}

TEST(BlendingSimulatorLattice, test_particle_volume)
{
	for (float angle : {30.0f, 45.0f, 59.9f}) {
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
	// Uncompressed lattice, so that touching spheres have exactly one diameter between their centers
	bs::SimulationParameters simulationParameters = latticeParameters(10.0f, 8.0f, 59.9f);
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
	EXPECT_NEAR(minimum, simulator.getParticleDiameter(), 1e-9);
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
	bs::BlendingSimulatorLattice<bs::AveragedParameters> native(latticeParameters(10.0f, static_cast<float>(8.0 * scale), 59.9f));
	ASSERT_NEAR(compressed.getParticleDiameter(), native.getParticleDiameter(), 1e-6);

	for (int i = 0; i < 400; i++) {
		compressed.stack(5.0f, 5.0f, bs::AveragedParameters(1.0 / 8.0, {1.0}));
		native.stack(5.0f, 5.0f, bs::AveragedParameters(1.0 / (8.0 * scale), {1.0}));
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
