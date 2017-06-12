#include "Processing.h"

#include <fstream>

#include "FlowLine.h"

ProcessingConfig::ProcessingConfig(
	unsigned int blendingBedLength,
	unsigned int blendingBedDepth,
	double reclaimSeconds,
	double flowFactor,
	double redCorrectionFactor,
	double blueCorrectionFactor,
	double yellowCorrectionFactor
)
	: blendingBedLength(blendingBedLength)
	, blendingBedDepth(blendingBedDepth)
	, reclaimSeconds(reclaimSeconds)
	, flowFactor(flowFactor)
	, redCorrectionFactor(redCorrectionFactor)
	, blueCorrectionFactor(blueCorrectionFactor)
	, yellowCorrectionFactor(yellowCorrectionFactor)
{
}

void processStackerFile(std::string& filename, const ProcessingConfig& config)
{
	// State variables
	FlowLine fl;
	uint64_t lastTimestamp = 0;
	double redSum = 0;
	double blueSum = 0;
	double yellowSum = 0;
	int lastRed = 0;
	int lastBlue = 0;
	int lastYellow = 0;
	float traversePathLengthAbs = float(config.blendingBedLength - config.blendingBedDepth);
	float traversePathOffsetAbs = float(config.blendingBedDepth) / 2.0f;

	std::ifstream inputFile(filename);

	if (!inputFile) {
		throw std::runtime_error("could not read input file");
	}

	// Read input from file
	while (inputFile >> fl) {
		// Convert variables to usable values
		double diff = double(fl.timestamp - lastTimestamp) / 1000000.0;
		redSum = redSum + fl.red * config.flowFactor * config.redCorrectionFactor * diff;
		blueSum = blueSum + fl.blue * config.flowFactor * config.blueCorrectionFactor * diff;
		yellowSum = yellowSum + fl.yellow * config.flowFactor * config.yellowCorrectionFactor * diff;
		fl.pos = std::min(std::max(0.0f, fl.pos), 1.0f);
		int posAbsolute = int(fl.pos * traversePathLengthAbs + traversePathOffsetAbs);

		// Drop particles
		std::cout
			<< posAbsolute << "\t"
			<< int(redSum) - lastRed << "\t"
			<< int(blueSum) - lastBlue << "\t"
			<< int(yellowSum) - lastYellow << "\n";

		// Save running variables for proper particle count calculation
		lastRed = int(redSum);
		lastBlue = int(blueSum);
		lastYellow = int(yellowSum);
		lastTimestamp = fl.timestamp;
	}

	std::cout << std::flush;
}

void processReclaimerFile(std::string& filename, const ProcessingConfig& config)
{
	// State variables
	FlowLine fl;
	uint64_t lastTimestamp = 0;
	double redSum = 0;
	double blueSum = 0;
	double yellowSum = 0;
	int lastRed = 0;
	int lastBlue = 0;
	int lastYellow = 0;

	std::ifstream inputFile(filename);

	if (!inputFile) {
		throw std::runtime_error("could not read input file");
	}

	// State variables
	int lastPosIndex = -1;

	// Read input from file
	while (inputFile >> fl) {
		// Ignore posRelative from file because it is too inconsistent (slowly updating, skipping a lot of positions)
		// Use time to generate position
		fl.pos = std::min(std::max(0.0f, float(double(fl.timestamp) / (1000000.0 * config.reclaimSeconds))), 1.0f);

		// Convert variables to usable values
		double diff = double(fl.timestamp - lastTimestamp) / 1000000.0;
		redSum = redSum + fl.red * config.flowFactor * config.redCorrectionFactor * diff;
		blueSum = blueSum + fl.blue * config.flowFactor * config.blueCorrectionFactor * diff;
		yellowSum = yellowSum + fl.yellow * config.flowFactor * config.yellowCorrectionFactor * diff;

		// Group material in same step width as simulator does (every mm)
		// Use posRelative for grouping and ignore sub-mm error
		int thisPosIndex = int(fl.pos * float(config.blendingBedLength));
		if (thisPosIndex != lastPosIndex) {
			if (lastPosIndex >= 0) {
				std::cout << lastPosIndex << "\t" << int(redSum) - lastRed << "\t" << int(blueSum) - lastBlue << "\t" << int(yellowSum) - lastYellow << "\n";

				// Save running variables for proper particle count calculation
				lastRed = int(redSum);
				lastBlue = int(blueSum);
				lastYellow = int(yellowSum);
			}

			lastPosIndex = thisPosIndex;
		}

		lastTimestamp = fl.timestamp;
	}

	std::cout << std::flush;
}
