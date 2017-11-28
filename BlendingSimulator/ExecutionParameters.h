#ifndef BLENDINGSIMULATOR_EXECUTIONPARAMETERS_H
#define BLENDINGSIMULATOR_EXECUTIONPARAMETERS_H

#include <string>

struct ExecutionParameters
{
	// Generic Options
	bool verbose = false;

	// Simulation Options
	bool detailed = false;

	// Visualization
	bool visualize = false;
	bool pretty = false;

	// Input / Output Options
	std::string heightsFile;
	std::string reclaimFile;
	float reclaimIncrement = 0.0f;
};

#endif
