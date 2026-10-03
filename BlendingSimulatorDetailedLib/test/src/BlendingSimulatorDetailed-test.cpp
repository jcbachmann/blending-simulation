#include <gtest/gtest.h>

#include "BlendingSimulator/BlendingSimulatorDetailed.h"
#include "BlendingSimulator/ParticleParameters.h"

namespace bs = blendingsimulator;

TEST(BlendingSimulatorDetailed, test_constructor_destructor)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 1.0f;
	simulationParameters.heapWorldSizeZ = 1.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.bulkDensityFactor = 1.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	simulationParameters.dropHeight = 10.0f;

	{
		bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(simulationParameters);
		std::pair<float, float> heapWorldSize = simulator.getHeapWorldSize();
		EXPECT_NEAR(heapWorldSize.first, simulationParameters.heapWorldSizeX, 0.1);
		EXPECT_NEAR(heapWorldSize.second, simulationParameters.heapWorldSizeZ, 0.1);
	}
}

TEST(BlendingSimulatorDetailed, test_is_paused)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 1.0f;
	simulationParameters.heapWorldSizeZ = 1.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.bulkDensityFactor = 1.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	simulationParameters.dropHeight = 10.0f;

	{
		bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(simulationParameters);
		EXPECT_FALSE(simulator.isPaused());
		simulator.pause();
		EXPECT_TRUE(simulator.isPaused());
		simulator.resume();
		EXPECT_FALSE(simulator.isPaused());
	}
}

TEST(BlendingSimulatorDetailed, test_heap_map)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 10.0f;
	simulationParameters.heapWorldSizeZ = 20.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.bulkDensityFactor = 1.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	simulationParameters.dropHeight = 10.0f;

	{
		bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(simulationParameters);
		std::pair<unsigned int, unsigned int> heapMapSize = simulator.getHeapMapSize();
		float* heapMap = simulator.getHeapMap();
		ASSERT_NE(heapMap, nullptr);
		for (unsigned int z = 0; z < heapMapSize.second; z++) {
			for (unsigned int x = 0; x < heapMapSize.first; x++) {
				EXPECT_NEAR(heapMap[z * heapMapSize.first + x], 0.0, 1e-10);
			}
		}
	}
}

TEST(BlendingSimulatorDetailed, test_stack_clear)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.bulkDensityFactor = 1.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	simulationParameters.dropHeight = 10.0f;

	{
		bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(simulationParameters);

		double volume = 1.0;
		bs::AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();
		simulator.clear();
		std::pair<unsigned int, unsigned int> heapMapSize = simulator.getHeapMapSize();
		float* heapMap = simulator.getHeapMap();
		ASSERT_NE(heapMap, nullptr);
		for (unsigned int zi = 0; zi < heapMapSize.second; zi++) {
			for (unsigned int xi = 0; xi < heapMapSize.first; xi++) {
				EXPECT_NEAR(heapMap[zi * heapMapSize.first + xi], 0.0, 1e-10);
			}
		}
	}
}

TEST(BlendingSimulatorDetailed, test_stack_reclaim)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.bulkDensityFactor = 1.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	simulationParameters.dropHeight = 10.0f;

	{
		bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(simulationParameters);

		double volume = 10.0;
		bs::AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut = simulator.reclaim(100);
		EXPECT_NEAR(pOut.getVolume(), volume, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}

TEST(BlendingSimulatorDetailed, test_particle_size_per_instance)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 10.0f;
	simulationParameters.heapWorldSizeZ = 10.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.bulkDensityFactor = 1.0f;
	simulationParameters.dropHeight = 2.0f;

	// A first simulator with 1 m particles must not influence the particles of later simulators
	{
		simulationParameters.particlesPerCubicMeter = 1.0f;
		bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(simulationParameters);
		simulator.stack(5.0f, 5.0f, bs::AveragedParameters(1.0, {1.0}));
		simulator.finishStacking();
	}

	// 0.5 m particles vary by 5 %
	simulationParameters.particlesPerCubicMeter = 8.0f;
	bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(simulationParameters);
	simulator.stack(5.0f, 5.0f, bs::AveragedParameters(1.0, {1.0}));
	simulator.finishStacking();

	ASSERT_EQ(simulator.inactiveOutputParticles.size(), 8);
	for (const auto* particle : simulator.inactiveOutputParticles) {
		for (double size : {particle->size.x, particle->size.y, particle->size.z}) {
			EXPECT_GE(size, 0.5 * 0.95 - 1e-6);
			EXPECT_LE(size, 0.5 * 1.05 + 1e-6);
		}
	}
}

std::vector<double> stackWithSeed(std::uint32_t seed)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 10.0f;
	simulationParameters.heapWorldSizeZ = 10.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.bulkDensityFactor = 1.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	simulationParameters.dropHeight = 2.0f;
	simulationParameters.seed = seed;

	bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(simulationParameters);
	simulator.stack(5.0f, 5.0f, bs::AveragedParameters(5.0, {1.0}));
	simulator.finishStacking();

	std::vector<double> positions;
	for (const auto* particle : simulator.inactiveOutputParticles) {
		positions.push_back(particle->position.x);
		positions.push_back(particle->position.y);
		positions.push_back(particle->position.z);
	}
	return positions;
}

TEST(BlendingSimulatorDetailed, test_seed_repeatable)
{
	auto positions = stackWithSeed(42);
	EXPECT_EQ(positions.size(), 15);
	EXPECT_EQ(positions, stackWithSeed(42));
}

TEST(BlendingSimulatorDetailed, test_seed_different)
{
	EXPECT_NE(stackWithSeed(1), stackWithSeed(2));
}
