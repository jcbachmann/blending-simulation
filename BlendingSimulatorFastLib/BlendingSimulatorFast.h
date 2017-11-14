#ifndef BlendingSimulatorFast_H
#define BlendingSimulatorFast_H

#include <vector>

#include "BlendingSimulator.h"

template<typename Parameters>
class BlendingSimulatorFast : public BlendingSimulator<Parameters>
{
	public:
		BlendingSimulatorFast(unsigned int heapLength, unsigned int heapDepth, unsigned int reclaimSlope,
							  bool fourDirectionsOnly = false);
		virtual void clear(void);
		virtual void stack(double position, const Parameters& parameters);
		virtual void finish(void);
		virtual float* getHeapMap(void);
		bool reclaim(int& position, Parameters& parameters, std::vector<int>& heights);
		void resetReclaimer(void);

	protected:

	private:
		// Dimensions of the simulated stockpile
		const unsigned int heapLength;
		const unsigned int heapDepth;

		// Slope used when reclaiming the stockpile
		const unsigned int reclaimSlope;

		// For every call of reclaim() the position is increased by one
		unsigned int reclaimerPos;

		// Variable tracking the height at each position for falling simulation
		std::vector<std::vector<int>> stackedHeights;

		// Variables for counting the particles per cross section for each color
		std::vector<Parameters> reclaimParameters;

		// Only allow particles to fall to axis aligned directions
		const bool fourDirectionsOnly;

		void updateHeapMap(void);
};

#include "BlendingSimulatorFast.impl.h"

#endif
