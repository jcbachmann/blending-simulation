#include <gtest/gtest.h>

#include "BlendingSimulatorFast.h"
#include "ParticleParameters.h"

TEST(BlendingSimulatorFast, test_constructor_destructor)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 1.0f;
	simulationParameters.heapWorldSizeZ = 1.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);
		std::pair<unsigned int, unsigned int> heapMapSize = simulator.getHeapMapSize();
		EXPECT_EQ(heapMapSize.first, 1);
		EXPECT_EQ(heapMapSize.second, 1);
		std::pair<float, float> heapWorldSize = simulator.getHeapWorldSize();
		EXPECT_NEAR(heapWorldSize.first, simulationParameters.heapWorldSizeX, 0.1);
		EXPECT_NEAR(heapWorldSize.second, simulationParameters.heapWorldSizeZ, 0.1);
	}
}

TEST(BlendingSimulatorFast, test_heap_size_1ppcm)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 100.0f;
	simulationParameters.heapWorldSizeZ = 200.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);
		std::pair<unsigned int, unsigned int> heapMapSize = simulator.getHeapMapSize();
		EXPECT_EQ(heapMapSize.first, 100);
		EXPECT_EQ(heapMapSize.second, 200);
	}
}

TEST(BlendingSimulatorFast, test_heap_size_8ppcm)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 100.0f;
	simulationParameters.heapWorldSizeZ = 200.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 8.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);
		std::pair<unsigned int, unsigned int> heapMapSize = simulator.getHeapMapSize();
		EXPECT_EQ(heapMapSize.first, 200);
		EXPECT_EQ(heapMapSize.second, 400);
		std::pair<float, float> heapWorldSize = simulator.getHeapWorldSize();
		EXPECT_NEAR(heapWorldSize.first, simulationParameters.heapWorldSizeX, 0.1);
		EXPECT_NEAR(heapWorldSize.second, simulationParameters.heapWorldSizeZ, 0.1);
	}
}

TEST(BlendingSimulatorFast, test_is_paused)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 1.0f;
	simulationParameters.heapWorldSizeZ = 1.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);
		EXPECT_FALSE(simulator.isPaused());
		simulator.pause();
		EXPECT_TRUE(simulator.isPaused());
		simulator.resume();
		EXPECT_FALSE(simulator.isPaused());
	}
}

TEST(BlendingSimulatorFast, test_heap_map)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 10.0f;
	simulationParameters.heapWorldSizeZ = 20.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);
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

TEST(BlendingSimulatorFast, test_stacking)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 90;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);

		double volume = 1.0;
		AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		std::pair<unsigned int, unsigned int> heapMapSize = simulator.getHeapMapSize();
		float* heapMap = simulator.getHeapMap();
		ASSERT_NE(heapMap, nullptr);
		for (unsigned int zi = 0; zi < heapMapSize.second; zi++) {
			for (unsigned int xi = 0; xi < heapMapSize.first; xi++) {
				if (xi == 1 && zi == 1) {
					EXPECT_NEAR(heapMap[zi * heapMapSize.first + xi], 1.0, 1e-10);
				} else {
					EXPECT_NEAR(heapMap[zi * heapMapSize.first + xi], 0.0, 1e-10);
				}
			}
		}
	}
}

TEST(BlendingSimulatorFast, test_clear)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 90;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);

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

TEST(BlendingSimulatorFast, test_reclaim)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 90;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);

		double volume = 1.0;
		AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut = simulator.reclaim(1.0);
		EXPECT_NEAR(pOut.getVolume(), 0, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut2 = simulator.reclaim(2.0);
		EXPECT_NEAR(pOut2.getVolume(), 1, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut3 = simulator.reclaim(3.0);
		EXPECT_NEAR(pOut3.getVolume(), 0, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}

TEST(BlendingSimulatorFast, test_reclaim_angle_90)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 90;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);

		double volume = 6.0 + 1e-3;
		AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut = simulator.reclaim(1.0);
		EXPECT_NEAR(pOut.getVolume(), 1, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut2 = simulator.reclaim(2.0);
		EXPECT_NEAR(pOut2.getVolume(), 4, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut3 = simulator.reclaim(3.0);
		EXPECT_NEAR(pOut3.getVolume(), 1, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}

TEST(BlendingSimulatorFast, test_reclaim_angle_45)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);

		double volume = 6.0;
		AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut = simulator.reclaim(1.0);
		EXPECT_NEAR(pOut.getVolume(), 2, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut2 = simulator.reclaim(2.0);
		EXPECT_NEAR(pOut2.getVolume(), 3, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut3 = simulator.reclaim(3.0);
		EXPECT_NEAR(pOut3.getVolume(), 1, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}

TEST(BlendingSimulatorFast, test_reclaim_eight)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 90;
	simulationParameters.eightLikelihood = 1.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);

		double volume = 9.0;
		AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut = simulator.reclaim(1.0);
		EXPECT_NEAR(pOut.getVolume(), 3, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut2 = simulator.reclaim(2.0);
		EXPECT_NEAR(pOut2.getVolume(), 3, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut3 = simulator.reclaim(3.0);
		EXPECT_NEAR(pOut3.getVolume(), 3, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}

TEST(BlendingSimulatorFast, test_reclaim_8ppcm)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 6.0f / 4.0f;
	simulationParameters.heapWorldSizeZ = 6.0f / 4.0f;
	simulationParameters.reclaimAngle = 90;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 8.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);

		double volume = 6.0 / 8.0;
		AveragedParameters p(volume, {1.0});

		float x = 2.0f / 4.0f;
		float z = 2.0f / 4.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut = simulator.reclaim(2.0f / 4.0f);
		EXPECT_NEAR(pOut.getVolume(), 1.0 / 8.0, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut2 = simulator.reclaim(4.0f / 4.0f);
		EXPECT_NEAR(pOut2.getVolume(), 4.0 / 8.0, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut3 = simulator.reclaim(6.0f / 4.0f);
		EXPECT_NEAR(pOut3.getVolume(), 1.0 / 8.0, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}

TEST(BlendingSimulatorFast, test_reclaim_angle_0)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 0.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);

		double volume = 6.0;
		AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut = simulator.reclaim(1.0);
		EXPECT_NEAR(pOut.getVolume(), 6, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut2 = simulator.reclaim(2.0);
		EXPECT_NEAR(pOut2.getVolume(), 0, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut3 = simulator.reclaim(3.0);
		EXPECT_NEAR(pOut3.getVolume(), 0, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}

TEST(BlendingSimulatorFast, test_reclaim_angle_180)
{
	SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 180.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);

		double volume = 6.0;
		AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut = simulator.reclaim(1.0);
		EXPECT_NEAR(pOut.getVolume(), 0, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut2 = simulator.reclaim(2.0);
		EXPECT_NEAR(pOut2.getVolume(), 0, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		AveragedParameters pOut3 = simulator.reclaim(3.0);
		EXPECT_NEAR(pOut3.getVolume(), 6, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}
