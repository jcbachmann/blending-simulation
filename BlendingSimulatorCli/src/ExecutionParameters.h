#ifndef BLENDINGSIMULATOR_EXECUTIONPARAMETERS_H
#define BLENDINGSIMULATOR_EXECUTIONPARAMETERS_H

#include <cmath>
#include <optional>
#include <string>

#include "BlendingSimulator/SimulationParameters.h"

struct ExecutionParameters
{
	// Generic Options
	bool verbose = false;

	// Simulation Options
	bool detailed = false;
	bool lattice = false;
	bool cone = false;
	bool formula = false;

#ifdef VISUALIZER_AVAILABLE
	// Visualization
	bool visualize = false;
	bool pretty = false;
#endif

	// Input / Output Options
	std::string heightsFile;
	std::string reclaimFile;
	// Reclaimer position increment in m, one particle size if not set
	std::optional<float> reclaimIncrement;
};

// The reclaimer position increment: the one given, or one particle size, the edge of a cube of the particle volume
inline float getReclaimIncrement(const ExecutionParameters& parameters, const blendingsimulator::SimulationParameters& simulationParameters)
{
	return parameters.reclaimIncrement.value_or(1.0f / std::cbrt(simulationParameters.particlesPerCubicMeter));
}

#endif
