#include <gtest/gtest.h>

#include "ExecutionParameters.h"

namespace bs = blendingsimulator;

TEST(ExecutionParameters, test_reclaim_increment_particle_size)
{
	ExecutionParameters parameters;
	bs::SimulationParameters simulationParameters;
	for (float ppm3 : {0.125f, 1.0f, 8.0f, 64.0f}) {
		simulationParameters.particlesPerCubicMeter = ppm3;
		EXPECT_NEAR(getReclaimIncrement(parameters, simulationParameters), 1.0 / std::cbrt(double(ppm3)), 1e-6);
	}
}

TEST(ExecutionParameters, test_reclaim_increment_given)
{
	ExecutionParameters parameters;
	parameters.reclaimIncrement = 2.0f;
	bs::SimulationParameters simulationParameters;
	simulationParameters.particlesPerCubicMeter = 8.0f;
	EXPECT_FLOAT_EQ(getReclaimIncrement(parameters, simulationParameters), 2.0f);
}
