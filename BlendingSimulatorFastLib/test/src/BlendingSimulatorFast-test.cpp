#include <cmath>

#include <gtest/gtest.h>

#include "BlendingSimulator/BlendingSimulatorFast.h"
#include "BlendingSimulator/ParticleParameters.h"
#include "BlendingSimulator/ReclaimSlices.h"

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
		EXPECT_NEAR(pOut.getVolume(), 1, 1.001e-3);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut2 = simulator.reclaim(2.0);
		EXPECT_NEAR(pOut2.getVolume(), 4, 1.001e-3);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut3 = simulator.reclaim(3.0);
		EXPECT_NEAR(pOut3.getVolume(), 1, 1.001e-3);
		// finishStacking stacked the remaining 0.001 m3 as a last, smaller particle, which falls into one of the columns
		EXPECT_NEAR(pOut.getVolume() + pOut2.getVolume() + pOut3.getVolume(), volume, 1e-10);

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

		// The center and its four neighbors on the ground, one more particle on top of the center. A cut at 45° spreads a ground
		// particle of column i half over slices i - 1 and i, the particle on top entirely over slices -1 and 0; slice -1 counts as 0.
		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut = simulator.reclaim(1.0);
		EXPECT_NEAR(pOut.getVolume(), 3.5, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut2 = simulator.reclaim(2.0);
		EXPECT_NEAR(pOut2.getVolume(), 2, 1e-10);

		EXPECT_FALSE(simulator.reclaimingFinished());
		bs::AveragedParameters pOut3 = simulator.reclaim(3.0);
		EXPECT_NEAR(pOut3.getVolume(), 0.5, 1e-10);

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

std::vector<float> stackConeWithSeed(std::optional<std::uint32_t> seed)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 20.0f;
	simulationParameters.heapWorldSizeZ = 20.0f;
	simulationParameters.reclaimAngle = 45.0;
	simulationParameters.eightLikelihood = 0.5f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	simulationParameters.seed = seed;

	bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);
	simulator.stack(10.0f, 10.0f, bs::AveragedParameters(500.0, {1.0}));
	simulator.finishStacking();

	std::pair<unsigned int, unsigned int> heapMapSize = simulator.getHeapMapSize();
	const float* heapMap = simulator.getHeapMap();
	std::vector<float> heights(heapMap, heapMap + heapMapSize.first * heapMapSize.second);

	// Append the reclaimed volumes to cover the reclaim slices as well
	float position = 0.0f;
	while (!simulator.reclaimingFinished()) {
		heights.push_back(static_cast<float>(simulator.reclaim(position).getVolume()));
		position += 1.0f;
	}

	return heights;
}

TEST(BlendingSimulatorFast, test_seed_repeatable)
{
	EXPECT_EQ(stackConeWithSeed(42), stackConeWithSeed(42));
}

TEST(BlendingSimulatorFast, test_seed_different)
{
	EXPECT_NE(stackConeWithSeed(1), stackConeWithSeed(2));
}

TEST(BlendingSimulatorFast, test_drop_position_cell)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 3.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	// Cell i covers the positions from i to i + 1, so positions up to just below the next integer stay in the cell
	for (float position : {1.0f, 1.5f, 1.99f}) {
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);
		simulator.stack(position, position, bs::AveragedParameters(1.0, {1.0}));
		std::pair<unsigned int, unsigned int> heapMapSize = simulator.getHeapMapSize();
		const float* heapMap = simulator.getHeapMap();
		for (unsigned int zi = 0; zi < heapMapSize.second; zi++) {
			for (unsigned int xi = 0; xi < heapMapSize.first; xi++) {
				EXPECT_NEAR(heapMap[zi * heapMapSize.first + xi], xi == 1 && zi == 1 ? 1.0 : 0.0, 1e-10) << "position " << position;
			}
		}
	}
}

TEST(BlendingSimulatorFast, test_reclaim_particle_center)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 4.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 45.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);

	// A particle on the ground in cell 2 has its center at x = 2.5 and y = 0.5, reclaimed at 2.0, spread over slices 1 and 2
	simulator.stack(2.5f, 1.5f, bs::AveragedParameters(1.0, {1.0}));
	EXPECT_NEAR(simulator.reclaim(1.0f).getVolume(), 0.0, 1e-10);
	EXPECT_NEAR(simulator.reclaim(2.0f).getVolume(), 0.5, 1e-10);
	EXPECT_NEAR(simulator.reclaim(3.0f).getVolume(), 0.5, 1e-10);
	EXPECT_NEAR(simulator.reclaim(4.0f).getVolume(), 0.0, 1e-10);
}

TEST(ReclaimSlices, test_spread_trapezoid)
{
	bs::ReclaimSlices<bs::AveragedParameters> slices;
	slices.resize(4, 1.0f);

	// Widths 0.5 and 1 around 2.25: ramps from 1.5 to 2.0 and from 2.5 to 3.0, so a quarter falls below 2.0
	slices.push(2.25, 0.5, 1.0, bs::AveragedParameters(1.0, {1.0}));
	// A uniform distribution of width 1 around 0.75, of which the part below 0 stays in the first slice
	slices.push(0.75, 1.0, 0.0, bs::AveragedParameters(2.0, {1.0}));
	// Around the circle: half of a uniform distribution around 0 goes into the last slice
	bs::ReclaimSlices<bs::AveragedParameters> circle;
	circle.resize(4, 1.0f);
	circle.push(0.0, 1.0, 0.0, bs::AveragedParameters(1.0, {1.0}), true);

	EXPECT_NEAR(slices.reclaim(1.0f).getVolume(), 1.5, 1e-10);
	EXPECT_NEAR(slices.reclaim(2.0f).getVolume(), 0.5 + 0.25, 1e-10);
	EXPECT_NEAR(slices.reclaim(3.0f).getVolume(), 0.75, 1e-10);
	EXPECT_NEAR(slices.reclaim(4.0f).getVolume(), 0.0, 1e-10);

	EXPECT_NEAR(circle.reclaim(1.0f).getVolume(), 0.5, 1e-10);
	EXPECT_NEAR(circle.reclaim(3.0f).getVolume(), 0.0, 1e-10);
	EXPECT_NEAR(circle.reclaim(4.0f).getVolume(), 0.5, 1e-10);
}

TEST(AveragedParameters, test_pop_without_volume_keeps_values_finite)
{
	// A slice can receive a share of zero volume; taking nothing from it must not divide zero by zero
	bs::AveragedParameters parameters(0.0, {2.0});
	EXPECT_NEAR(parameters.pop(0.0).getVolume(), 0.0, 1e-10);
	parameters.push(1.0, {3.0});
	EXPECT_NEAR(parameters.getValue(0), 3.0, 1e-10);
}

TEST(BlendingSimulatorFast, test_reclaimed_parameters_finite)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 59.0f;
	simulationParameters.heapWorldSizeZ = 20.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;
	simulationParameters.seed = 3;

	bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);
	for (int i = 0; i < 50; i++) {
		simulator.stack(10.0f + 39.0f * float(i % 7) / 6.0f, 10.0f, bs::AveragedParameters(50.0, {double(i % 5)}));
	}

	double volume = 0.0;
	float position = 0.0f;
	while (!simulator.reclaimingFinished()) {
		// Also beyond the end of the pile, where slices are empty: the Python bindings return getValues() of every slice
		bs::AveragedParameters reclaimed = simulator.reclaim(position);
		for (double value : reclaimed.getValues()) {
			EXPECT_TRUE(std::isfinite(value)) << "position " << position;
		}
		volume += reclaimed.getVolume();
		position += 1.0f;
	}
	EXPECT_NEAR(volume, 2500.0, 1e-6);
}

// Reclaims everything in steps of 1 m like the Python bindings and returns the total volume
double reclaimTotal(bs::BlendingSimulator<bs::AveragedParameters>& simulator)
{
	simulator.finishStacking();
	double total = 0.0;
	float position = 0.0f;
	while (!simulator.reclaimingFinished()) {
		total += simulator.reclaim(position).getVolume();
		position += 1.0f;
	}
	return total;
}

TEST(BlendingSimulatorFast, test_no_volume_lost)
{
	// Portions that are not whole particles: what stays in the buffer is stacked by finishStacking as a last, smaller particle
	for (float ppm3 : {1.0f, 4.0f, 16.0f}) {
		bs::SimulationParameters simulationParameters;
		simulationParameters.heapWorldSizeX = 59.0f;
		simulationParameters.heapWorldSizeZ = 20.0f;
		simulationParameters.particlesPerCubicMeter = ppm3;
		simulationParameters.seed = 3;
		bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);
		double stacked = 0.0;
		for (int k = 0; k < 37; k++) {
			const double volume = 1.37 + 0.11 * k;
			simulator.stack(float(12.0 + k), 10.0f, bs::AveragedParameters(volume, {1.0}));
			stacked += volume;
		}
		EXPECT_NEAR(reclaimTotal(simulator), stacked, 1e-6) << "ppm3 " << ppm3;
	}
}

TEST(BlendingSimulatorFast, test_no_volume_lost_circular)
{
	// On a circular bed the slices do not end on a 1 m reclaim step: the last step takes the rest of the last slice
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 60.0f;
	simulationParameters.heapWorldSizeZ = 60.0f;
	simulationParameters.particlesPerCubicMeter = 16.0f;
	simulationParameters.circular = true;
	simulationParameters.seed = 5;
	bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);
	const double pi = std::atan(1.0) * 4.0;
	for (int k = 0; k < 200; k++) {
		const double angle = 2.0 * pi * k / 50.0;
		simulator.stack(float(30.0 + 15.0 * std::cos(angle)), float(30.0 + 15.0 * std::sin(angle)), bs::AveragedParameters(5.0, {1.0}));
	}
	EXPECT_NEAR(reclaimTotal(simulator), 1000.0, 1e-6);
}
