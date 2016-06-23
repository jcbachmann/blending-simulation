#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>

#include "FlowLine.h"
#include "Simulator.h"
#include "ConfigHandler.h"

struct ProcessingConfig {
		// Simulator stockpile dimensions (traverse path range)
		int stockpileLength;
		int stockpileWidth;

		// Slope used when reclaiming the stockpile
		unsigned int reclaimSlope;

		// Offset subtracted from the reclaiming stockpile position to match stacking positions
		float reclaimPosOffset;

		// Factor by which the relative amount is multiplied to get amount of particles for simulator
		double flowFactor;

		// Correction factors for the different colors to compensate for acquisition errors
		float redCorrectionFactor;
		float blueCorrectionFactor;
		float yellowCorrectionFactor;
};

void processStackerFile(std::string& filename, ProcessingConfig& config)
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
	Simulator simulator(config.stockpileLength + config.stockpileWidth, config.stockpileWidth, config.reclaimSlope);

	std::ifstream inputFile(filename);

	if (!inputFile) {
		throw std::runtime_error("Could not read file");
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
		simulator.stack(posAbsolute + config.stockpileWidth / 2, int(redSum) - lastRed, int(blueSum) - lastBlue, int(yellowSum) - lastYellow);

		// Save running variables for proper particle count calculation
		lastRed = int(redSum);
		lastBlue = int(blueSum);
		lastYellow = int(yellowSum);
		lastTimestamp = fl.timestamp;
	}

	std::ofstream fPile(filename + ".sim.pile.csv");
	if (!fPile) {
		throw std::runtime_error("Could not open pile reclaim file stream");
	}

	std::ofstream fPilePlotData(filename + ".sim.pile.plotdata");
	if (!fPilePlotData) {
		throw std::runtime_error("Could not open pile reclaim plot data file stream");
	}

	std::ofstream fSlices(filename + ".sim.slices.csv");
	if (!fSlices) {
		throw std::runtime_error("Could not open slices reclaim file stream");
	}
	fSlices << "pos\tred\tblue\tyellow\n";

	int posAbsolute;
	int redCount;
	int blueCount;
	int yellowCount;
	std::vector<int> heights;
	int i = 0;

	while (simulator.reclaim(posAbsolute, redCount, blueCount, yellowCount, heights)) {
		float posRelative = float(posAbsolute - config.stockpileWidth / 2) / float(config.stockpileLength);
		fPile << posRelative << "\t";
		for (int j = 0; j < config.stockpileWidth; j++) {
			fPile << heights[j] << "\t";
			if ((j > 0 && heights[j - 1]) || heights[j] || (j < config.stockpileWidth - 1 && heights[j + 1])) {
				fPilePlotData << i << " " << j << " " << heights[j] << "\n";
			}
		}
		fPile << "\n";
		fPilePlotData << "\n";
		fSlices << posRelative << "\t" << redCount << "\t" << blueCount << "\t"  << yellowCount << "\n";
		i++;
	}
}

void processReclaimerFile(std::string& filename, ProcessingConfig& config)
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
		throw std::runtime_error("Could not read file");
	}

	std::ofstream fSlices(filename + ".slices.csv");
	if (!fSlices) {
		throw std::runtime_error("Could not open slices reclaim file stream");
	}
	fSlices << "pos\tred\tblue\tyellow\n";

	// State variables
	int lastPosIndex = -1;

	// Read input from file
	while (inputFile >> fl) {
		// Ignore posRelative from file because it is too inconsistent (slowly updating, skipping a lot of positions)
		// Use time to generate position
		fl.pos = 1.0 + double(fl.timestamp) / 600000000.0;

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
				fSlices << (float(lastPosIndex) / float(config.stockpileLength) - config.reclaimPosOffset) << "\t" << int(redSum) - lastRed << "\t" << int(blueSum) - lastBlue << "\t" << int(yellowSum) - lastYellow << "\n";

				// Save running variables for proper particle count calculation
				lastRed = int(redSum);
				lastBlue = int(blueSum);
				lastYellow = int(yellowSum);
			}

			lastPosIndex = thisPosIndex;
		}

		lastTimestamp = fl.timestamp;
	}
}

int main(int argc, char** argv) try
{
	if (argc < 2) {
		std::cout << "Invalid argument count, usage: " << argv[0] << " <stacker/reclaimer results files>" << std::endl;
		return 1;
	}

	ProcessingConfig config;
	{
		ConfigHandler configHandler("processing.config");
		config.stockpileLength = configHandler.get("stockpile length", 1000);
		config.stockpileWidth = configHandler.get("stockpile width", 250);
		config.reclaimSlope = configHandler.get("reclaim slope", 1u);
		config.reclaimPosOffset = configHandler.get("reclaim pos offset", 1.0f);
		config.flowFactor = configHandler.get("flow factor", 100000.0);
		config.redCorrectionFactor = configHandler.get("red correction factor", 1.0f);
		config.blueCorrectionFactor = configHandler.get("blue correction factor", 1.0f);
		config.yellowCorrectionFactor = configHandler.get("yellow correction factor", 1.0f);
	}

	for (int i = 1; i < argc; i++) {
		std::string filename = argv[i];
		std::cout << "Processing " << filename << std::endl;

		if (filename.find("stack.csv") != std::string::npos) {
			processStackerFile(filename, config);
		} else if (filename.find("reclaim.csv") != std::string::npos) {
			processReclaimerFile(filename, config);
		} else {
			throw std::runtime_error("Unknown file type");
		}
	}
} catch (std::exception& e) {
	std::cerr << e.what() << std::endl;
	return 1;
}
