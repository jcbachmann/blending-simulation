#ifndef BLENDINGSIMULATOR_EXECUTIONPARAMETERS_H
#define BLENDINGSIMULATOR_EXECUTIONPARAMETERS_H

#include <string>

class ExecutionParameters
{
	public:
		// Generic Options
		bool verbose = false;

		// Simulation Options
		bool detailed = false;
		float length = 0;
		float depth = 0;
		float reclaimAngle = 0;
		float eightLikelihood = 0.0f;
		float bulkDensity = 0.0f;
		float particlesPerCubicMeter = 0.0f;
		float dropHeight = 0.0f;
		float reclaimIncrement = 0.0f;

		// Visualization
		bool visualize = false;
		bool pretty = false;

		// Input / Output Options
		unsigned int parameterCount = 0;
		std::string heightsFile;
		std::string reclaimFile;
};

#endif
