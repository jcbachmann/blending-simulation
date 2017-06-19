#ifndef BLENDINGSIMULATOR_EXECUTIONPARAMETERS_H
#define BLENDINGSIMULATOR_EXECUTIONPARAMETERS_H

class ExecutionParameters
{
	public:
		bool detailed = false;
		bool printHeights = false;
		bool skipReclaim = false;
		bool skipPos = false;
		bool fourDirectionsOnly = false;
		bool useCounting = false;
		unsigned int parameterCount;
		unsigned int length;
		unsigned int depth;
		unsigned int slope;
};

#endif
