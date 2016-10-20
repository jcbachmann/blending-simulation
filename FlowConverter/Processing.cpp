#include "Processing.h"

#include <fstream>

#include "FlowLine.h"

ProcessingConfig::ProcessingConfig(
		unsigned int stockpileLength,
		unsigned int stockpileDepth,
		double reclaimSeconds,
		double flowFactor,
		double redCorrectionFactor,
		double blueCorrectionFactor,
		double yellowCorrectionFactor
)
	: stockpileLength(stockpileLength)
	, stockpileDepth(stockpileDepth)
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
		int posAbsolute = int(fl.pos * float(config.stockpileLength));
		if (posAbsolute < 0) posAbsolute = 0;
		if (posAbsolute >= config.stockpileLength) posAbsolute = config.stockpileLength - 1;

		// Drop particles
		std::cout
				<< posAbsolute + config.stockpileDepth / 2 << "\t"
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
		fl.pos = float(double(fl.timestamp) / (1000000.0 * config.reclaimSeconds));

		// Convert variables to usable values
		double diff = double(fl.timestamp - lastTimestamp) / 1000000.0;
		redSum = redSum + fl.red * config.flowFactor * config.redCorrectionFactor * diff;
		blueSum = blueSum + fl.blue * config.flowFactor * config.blueCorrectionFactor * diff;
		yellowSum = yellowSum + fl.yellow * config.flowFactor * config.yellowCorrectionFactor * diff;

		// Group material in same step width as simulator does (every mm)
		// Use posRelative for grouping and ignore sub-mm error
		int thisPosIndex = int(fl.pos * float(config.stockpileLength));
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
