#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "BlendingSimulator/BlendingSimulatorFast.h"
#include "BlendingSimulator/ParticleParameters.h"
#include "HeightMap.h"

namespace bs = blendingsimulator;

std::vector<std::vector<float>> parseHeightMap(const std::string& text)
{
	std::vector<std::vector<float>> rows;
	std::istringstream lines(text);
	std::string line;
	while (std::getline(lines, line)) {
		std::istringstream values(line);
		std::vector<float> row;
		float value;
		while (values >> value) {
			row.push_back(value);
		}
		rows.push_back(row);
	}
	return rows;
}

TEST(HeightMap, test_size)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 4.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);

	std::ostringstream out;
	writeHeightMap(out, simulator);
	auto rows = parseHeightMap(out.str());

	ASSERT_EQ(rows.size(), 3);
	for (const auto& row : rows) {
		EXPECT_EQ(row.size(), 4);
	}
}

TEST(HeightMap, test_particle_position)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = 4.0f;
	simulationParameters.heapWorldSizeZ = 3.0f;
	simulationParameters.reclaimAngle = 90.0f;
	simulationParameters.eightLikelihood = 0.0f;
	simulationParameters.particlesPerCubicMeter = 1.0f;

	bs::BlendingSimulatorFast<bs::AveragedParameters> simulator(simulationParameters);

	// A single particle on the empty bed stays in the cell it is dropped on
	simulator.stack(2.0f, 1.0f, bs::AveragedParameters(1.0, {1.0}));
	simulator.finishStacking();

	std::ostringstream out;
	writeHeightMap(out, simulator);
	auto rows = parseHeightMap(out.str());

	ASSERT_EQ(rows.size(), 3);
	for (unsigned int z = 0; z < rows.size(); z++) {
		ASSERT_EQ(rows[z].size(), 4);
		for (unsigned int x = 0; x < rows[z].size(); x++) {
			EXPECT_NEAR(rows[z][x], (x == 2 && z == 1) ? 1.0 : 0.0, 1e-6) << "at x=" << x << " z=" << z;
		}
	}
}
