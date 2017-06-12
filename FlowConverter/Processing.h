#ifndef PROCESSING_H
#define PROCESSING_H

#include <string>

struct ProcessingConfig
{
	// Simulator blending bed dimensions
	unsigned int blendingBedLength;
	unsigned int blendingBedDepth;

	// Duration it took to fully reclaim the stockpile (used for fine position generation)
	double reclaimSeconds;

	// Factor by which the relative amount is multiplied to get amount of particles for simulator
	double flowFactor;

	// Correction factors for the different colors to compensate for acquisition errors
	double redCorrectionFactor;
	double blueCorrectionFactor;
	double yellowCorrectionFactor;

	ProcessingConfig(
		unsigned int blendingBedLength,
		unsigned int blendingBedDepth,
		double reclaimSeconds,
		double flowFactor,
		double redCorrectionFactor,
		double blueCorrectionFactor,
		double yellowCorrectionFactor
	);
};

void processStackerFile(std::string& filename, const ProcessingConfig& config);
void processReclaimerFile(std::string& filename, const ProcessingConfig& config);

#endif
