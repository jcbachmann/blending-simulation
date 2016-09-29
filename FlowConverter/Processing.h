#ifndef PROCESSING_H
#define PROCESSING_H

#include <string>

struct ProcessingConfig {
		// Simulator stockpile dimensions (traverse path range)
		unsigned int stockpileLength;
		unsigned int stockpileWidth;

		// Slope used when reclaiming the stockpile
		unsigned int reclaimSlope;

		// Duration it took to fully reclaim the stockpile (used for fine position generation)
		double reclaimSeconds;

		// Factor by which the relative amount is multiplied to get amount of particles for simulator
		double flowFactor;

		// Correction factors for the different colors to compensate for acquisition errors
		float redCorrectionFactor;
		float blueCorrectionFactor;
		float yellowCorrectionFactor;

		ProcessingConfig(std::string filename);
};

void processStackerFile(std::string& filename, ProcessingConfig& config);
void processReclaimerFile(std::string& filename, ProcessingConfig& config);

#endif
