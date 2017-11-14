#include <gtest/gtest.h>

#include <BlendingSimulatorFast.h>

TEST(BlendingSimulatorFast, test_constructor_destructor)
{
	float heapWorldSizeX = 1.0f;
	float heapWorldSizeZ = 1.0f;
	float reclaimAngle = 45.0;
	float eightLikelihood = 0.0f;
	float particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, eightLikelihood, particlesPerCubicMeter, false);
		std::pair<unsigned int, unsigned int> heapMapSize = simulator.getHeapMapSize();
		EXPECT_EQ(heapMapSize.first, 1);
		EXPECT_EQ(heapMapSize.second, 1);
		std::pair<float, float> heapWorldSize = simulator.getHeapWorldSize();
		EXPECT_NEAR(heapWorldSize.first, heapWorldSizeX, 0.1);
		EXPECT_NEAR(heapWorldSize.second, heapWorldSizeZ, 0.1);
	}
}

TEST(BlendingSimulatorFast, test_heap_size_1ppcm)
{
	float heapWorldSizeX = 100.0f;
	float heapWorldSizeZ = 200.0f;
	float reclaimAngle = 45.0;
	float eightLikelihood = 0.0f;
	float particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, eightLikelihood, particlesPerCubicMeter, false);
		std::pair<unsigned int, unsigned int> heapMapSize = simulator.getHeapMapSize();
		EXPECT_EQ(heapMapSize.first, 100);
		EXPECT_EQ(heapMapSize.second, 200);
	}
}

TEST(BlendingSimulatorFast, test_heap_size_8ppcm)
{
	float heapWorldSizeX = 100.0f;
	float heapWorldSizeZ = 200.0f;
	float reclaimAngle = 45.0;
	float eightLikelihood = 0.0f;
	float particlesPerCubicMeter = 8.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, eightLikelihood, particlesPerCubicMeter, false);
		std::pair<unsigned int, unsigned int> heapMapSize = simulator.getHeapMapSize();
		EXPECT_EQ(heapMapSize.first, 200);
		EXPECT_EQ(heapMapSize.second, 400);
		std::pair<float, float> heapWorldSize = simulator.getHeapWorldSize();
		EXPECT_NEAR(heapWorldSize.first, heapWorldSizeX, 0.1);
		EXPECT_NEAR(heapWorldSize.second, heapWorldSizeZ, 0.1);
	}
}

TEST(BlendingSimulatorFast, test_is_paused)
{
	float heapWorldSizeX = 1.0f;
	float heapWorldSizeZ = 1.0f;
	float reclaimAngle = 45.0;
	float eightLikelihood = 0.0f;
	float particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, eightLikelihood, particlesPerCubicMeter, false);
		EXPECT_FALSE(simulator.isPaused());
		simulator.pause();
		EXPECT_TRUE(simulator.isPaused());
		simulator.resume();
		EXPECT_FALSE(simulator.isPaused());
	}
}

TEST(BlendingSimulatorFast, test_heap_map)
{
	float heapWorldSizeX = 10.0f;
	float heapWorldSizeZ = 20.0f;
	float reclaimAngle = 45.0;
	float eightLikelihood = 0.0f;
	float particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, eightLikelihood, particlesPerCubicMeter, false);
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
	float heapWorldSizeX = 3.0f;
	float heapWorldSizeZ = 3.0f;
	float reclaimAngle = 90;
	float eightLikelihood = 0.0f;
	float particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, eightLikelihood, particlesPerCubicMeter, false);

		double volume = 1.0;
		AveragedParameters p(volume, {1.0});

		float position = 1.0f;
		simulator.stack(position, p);
		simulator.finishStacking();

		std::pair<unsigned int, unsigned int> heapMapSize = simulator.getHeapMapSize();
		float* heapMap = simulator.getHeapMap();
		ASSERT_NE(heapMap, nullptr);
		for (unsigned int z = 0; z < heapMapSize.second; z++) {
			for (unsigned int x = 0; x < heapMapSize.first; x++) {
				if (x == 1 && z == 1) {
					EXPECT_NEAR(heapMap[z * heapMapSize.first + x], 1.0, 1e-10);
				} else {
					EXPECT_NEAR(heapMap[z * heapMapSize.first + x], 0.0, 1e-10);
				}
			}
		}
	}
}

TEST(BlendingSimulatorFast, test_clear)
{
	float heapWorldSizeX = 3.0f;
	float heapWorldSizeZ = 3.0f;
	float reclaimAngle = 90;
	float eightLikelihood = 0.0f;
	float particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, eightLikelihood, particlesPerCubicMeter, false);

		double volume = 1.0;
		AveragedParameters p(volume, {1.0});

		float position = 1.0f;
		simulator.stack(position, p);
		simulator.finishStacking();
		simulator.clear();
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

TEST(BlendingSimulatorFast, test_reclaim)
{
	float heapWorldSizeX = 3.0f;
	float heapWorldSizeZ = 3.0f;
	float reclaimAngle = 90;
	float eightLikelihood = 0.0f;
	float particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, eightLikelihood, particlesPerCubicMeter, false);

		double volume = 1.0;
		AveragedParameters p(volume, {1.0});

		float position = 1.0f;
		simulator.stack(position, p);
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
	float heapWorldSizeX = 3.0f;
	float heapWorldSizeZ = 3.0f;
	float reclaimAngle = 90;
	float eightLikelihood = 0.0f;
	float particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, eightLikelihood, particlesPerCubicMeter, false);

		double volume = 6.0 + 1e-3;
		AveragedParameters p(volume, {1.0});

		float position = 1.0f;
		simulator.stack(position, p);
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
	float heapWorldSizeX = 3.0f;
	float heapWorldSizeZ = 3.0f;
	float reclaimAngle = 45.0;
	float eightLikelihood = 0.0f;
	float particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, eightLikelihood, particlesPerCubicMeter, false);

		double volume = 6.0;
		AveragedParameters p(volume, {1.0});

		float position = 1.0f;
		simulator.stack(position, p);
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
	float heapWorldSizeX = 3.0f;
	float heapWorldSizeZ = 3.0f;
	float reclaimAngle = 90;
	float eightLikelihood = 1.0f;
	float particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, eightLikelihood, particlesPerCubicMeter, false);

		double volume = 9.0;
		AveragedParameters p(volume, {1.0});

		float position = 1.0f;
		simulator.stack(position, p);
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
	float heapWorldSizeX = 6.0f / 4.0f;
	float heapWorldSizeZ = 6.0f / 4.0f;
	float reclaimAngle = 90;
	float eightLikelihood = 0.0f;
	float particlesPerCubicMeter = 8.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, eightLikelihood, particlesPerCubicMeter, false);

		double volume = 6.0 / 8.0;
		AveragedParameters p(volume, {1.0});

		float position = 2.0f / 4.0f;
		simulator.stack(position, p);
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
	float heapWorldSizeX = 3.0f;
	float heapWorldSizeZ = 3.0f;
	float reclaimAngle = 0.0;
	float eightLikelihood = 0.0f;
	float particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, eightLikelihood, particlesPerCubicMeter, false);

		double volume = 6.0;
		AveragedParameters p(volume, {1.0});

		float position = 1.0f;
		simulator.stack(position, p);
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
	float heapWorldSizeX = 3.0f;
	float heapWorldSizeZ = 3.0f;
	float reclaimAngle = 180.0;
	float eightLikelihood = 0.0f;
	float particlesPerCubicMeter = 1.0f;

	{
		BlendingSimulatorFast<AveragedParameters> simulator(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, eightLikelihood, particlesPerCubicMeter, false);

		double volume = 6.0;
		AveragedParameters p(volume, {1.0});

		float position = 1.0f;
		simulator.stack(position, p);
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
