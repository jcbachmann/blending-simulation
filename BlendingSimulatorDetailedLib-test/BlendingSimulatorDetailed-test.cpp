#include <gtest/gtest.h>

#include "BlendingSimulatorDetailed.h"
#include "ParticleParameters.h"

TEST(BlendingSimulatorDetailed, test_constructor_destructor)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 1.0f;
	simulationParameters.heapWorldSizeZ = 1.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.bulkDensityFactor = 1.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	simulationParameters.dropHeight = 10.0f;

	{
		BlendingSimulatorDetailed<AveragedParameters> simulator(simulationParameters);
		std::pair<float, float> heapWorldSize = simulator.getHeapWorldSize();
		EXPECT_NEAR(heapWorldSize.first, simulationParameters.heapWorldSizeX, 0.1);
		EXPECT_NEAR(heapWorldSize.second, simulationParameters.heapWorldSizeZ, 0.1);
	}
}

TEST(BlendingSimulatorDetailed, test_is_paused)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 1.0f;
	simulationParameters.heapWorldSizeZ = 1.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.bulkDensityFactor = 1.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	simulationParameters.dropHeight = 10.0f;

	{
		BlendingSimulatorDetailed<AveragedParameters> simulator(simulationParameters);
		EXPECT_FALSE(simulator.isPaused());
		simulator.pause();
		EXPECT_TRUE(simulator.isPaused());
		simulator.resume();
		EXPECT_FALSE(simulator.isPaused());
	}
}

TEST(BlendingSimulatorDetailed, test_heap_map)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 10.0f;
	simulationParameters.heapWorldSizeZ = 20.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.bulkDensityFactor = 1.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	simulationParameters.dropHeight = 10.0f;

	{
		BlendingSimulatorDetailed<AveragedParameters> simulator(simulationParameters);
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
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.bulkDensityFactor = 1.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	simulationParameters.dropHeight = 10.0f;

	{
		BlendingSimulatorDetailed<AveragedParameters> simulator(simulationParameters);

		double volume = 1.0;
		AveragedParameters p(volume, {1.0});

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
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.bulkDensityFactor = 1.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	simulationParameters.dropHeight = 10.0f;

	{
		BlendingSimulatorDetailed<AveragedParameters> simulator(simulationParameters);

		double volume = 10.0;
		AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut = simulator.reclaim(100);
		EXPECT_NEAR(pOut.getVolume(), volume, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}
