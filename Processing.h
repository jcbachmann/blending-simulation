#ifndef PROCESSING_H
#define PROCESSING_H

#include <string>

struct ProcessingConfig {
		// Simulator stockpile dimensions (traverse path range)
		unsigned int stockpileLength;
		unsigned int stockpileWidth;

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

		ProcessingConfig(std::string filename);
};

void processStackerFile(std::string& filename, ProcessingConfig& config);
void processReclaimerFile(std::string& filename, ProcessingConfig& config);

#endif
