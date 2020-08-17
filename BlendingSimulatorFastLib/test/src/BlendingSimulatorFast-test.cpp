#include <gtest/gtest.h>

#include "BlendingSimulator/BlendingSimulatorFast.h"
#include "BlendingSimulator/ParticleParameters.h"

namespace bs = blendingsimulator;

TEST(BlendingSimulatorFast, test_constructor_destructor)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 1.0f;
	simulationParameters.heapWorldSizeZ = 1.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);
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
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 100.0f;
	simulationParameters.heapWorldSizeZ = 200.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);
		std::pair<unsigned int, unsigned int> heapMapSize = simulator.getHeapMapSize();
		EXPECT_EQ(heapMapSize.first, 100);
		EXPECT_EQ(heapMapSize.second, 200);
	}
}

TEST(BlendingSimulatorFast, test_heap_size_8ppcm)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 100.0f;
	simulationParameters.heapWorldSizeZ = 200.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 8.0f;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);
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
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 1.0f;
	simulationParameters.heapWorldSizeZ = 1.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);
		EXPECT_FALSE(simulator.isPaused());
		simulator.pause();
		EXPECT_TRUE(simulator.isPaused());
		simulator.resume();
		EXPECT_FALSE(simulator.isPaused());
	}
}

TEST(BlendingSimulatorFast, test_heap_map)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 10.0f;
	simulationParameters.heapWorldSizeZ = 20.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);
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
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 90;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);

		double volume = 1.0;
		bs::AveragedParameters p(volume, {1.0});

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
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 90;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);

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

TEST(BlendingSimulatorFast, test_reclaim)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 90;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);

		double volume = 1.0;
		bs::AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut = simulator.reclaim(1.0);
		EXPECT_NEAR(pOut.getVolume(), 0, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut2 = simulator.reclaim(2.0);
		EXPECT_NEAR(pOut2.getVolume(), 1, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut3 = simulator.reclaim(3.0);
		EXPECT_NEAR(pOut3.getVolume(), 0, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}

TEST(BlendingSimulatorFast, test_reclaim_angle_90)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 90;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);

		double volume = 6.0 + 1e-3;
		bs::AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut = simulator.reclaim(1.0);
		EXPECT_NEAR(pOut.getVolume(), 1, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut2 = simulator.reclaim(2.0);
		EXPECT_NEAR(pOut2.getVolume(), 4, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut3 = simulator.reclaim(3.0);
		EXPECT_NEAR(pOut3.getVolume(), 1, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}

TEST(BlendingSimulatorFast, test_reclaim_angle_45)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);

		double volume = 6.0;
		bs::AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut = simulator.reclaim(1.0);
		EXPECT_NEAR(pOut.getVolume(), 2, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut2 = simulator.reclaim(2.0);
		EXPECT_NEAR(pOut2.getVolume(), 3, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut3 = simulator.reclaim(3.0);
		EXPECT_NEAR(pOut3.getVolume(), 1, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}

TEST(BlendingSimulatorFast, test_reclaim_eight)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 90;
	simulationParameters.eightLikelihood = 1.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);

		double volume = 9.0;
		bs::AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut = simulator.reclaim(1.0);
		EXPECT_NEAR(pOut.getVolume(), 3, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut2 = simulator.reclaim(2.0);
		EXPECT_NEAR(pOut2.getVolume(), 3, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut3 = simulator.reclaim(3.0);
		EXPECT_NEAR(pOut3.getVolume(), 3, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}

TEST(BlendingSimulatorFast, test_reclaim_8ppcm)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 6.0f / 4.0f;
	simulationParameters.heapWorldSizeZ = 6.0f / 4.0f;
	simulationParameters.reclaimAngle = 90;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 8.0f;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);

		double volume = 6.0 / 8.0;
		bs::AveragedParameters p(volume, {1.0});

		float x = 2.0f / 4.0f;
		float z = 2.0f / 4.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut = simulator.reclaim(2.0f / 4.0f);
		EXPECT_NEAR(pOut.getVolume(), 1.0 / 8.0, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut2 = simulator.reclaim(4.0f / 4.0f);
		EXPECT_NEAR(pOut2.getVolume(), 4.0 / 8.0, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut3 = simulator.reclaim(6.0f / 4.0f);
		EXPECT_NEAR(pOut3.getVolume(), 1.0 / 8.0, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}

TEST(BlendingSimulatorFast, test_reclaim_angle_0)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 0.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);

		double volume = 6.0;
		bs::AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut = simulator.reclaim(1.0);
		EXPECT_NEAR(pOut.getVolume(), 6, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut2 = simulator.reclaim(2.0);
		EXPECT_NEAR(pOut2.getVolume(), 0, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut3 = simulator.reclaim(3.0);
		EXPECT_NEAR(pOut3.getVolume(), 0, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}

TEST(BlendingSimulatorFast, test_reclaim_angle_180)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 180.0;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);

		double volume = 6.0;
		bs::AveragedParameters p(volume, {1.0});

		float x = 1.0f;
		float z = 1.0f;
		simulator.stack(x, z, p);
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut = simulator.reclaim(1.0);
		EXPECT_NEAR(pOut.getVolume(), 0, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut2 = simulator.reclaim(2.0);
		EXPECT_NEAR(pOut2.getVolume(), 0, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut3 = simulator.reclaim(3.0);
		EXPECT_NEAR(pOut3.getVolume(), 6, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}

TEST(BlendingSimulatorFast, test_circular)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 10.0f;
	simulationParameters.heapWorldSizeZ = 10.0f;
	simulationParameters.reclaimAngle = 90;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	simulationParameters.circular = true;

	{
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);

		//        X
		//     0 1 . 8 9
		//   0 3 . . . 2
		// Z 1 . . . . .
		//   . . . . . .
		//   8 . . . . .
		//   9 0 . . . 1
		const float pi = 3.141592653589793238463;
		float total_way = 2.0f * pi * 2.5f;
		simulator.stack(0.0, 0.0, {3.0, {1.0}});
		simulator.stack(9.0, 0.0, {2.0, {1.0}});
		simulator.stack(9.0, 9.0, {1.0, {1.0}});
		simulator.finishStacking();

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut = simulator.reclaim(0.25f * total_way);
		EXPECT_NEAR(pOut.getVolume(), 0, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut2 = simulator.reclaim(0.5f * total_way);
		EXPECT_NEAR(pOut2.getVolume(), 1, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut3 = simulator.reclaim(0.75f * total_way);
		EXPECT_NEAR(pOut3.getVolume(), 2, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut4 = simulator.reclaim(1.0f * total_way);
		EXPECT_NEAR(pOut4.getVolume(), 3, 1e-10);

		EXPECT_TRUE(simulator.reclaimingFinished());
	}
}
