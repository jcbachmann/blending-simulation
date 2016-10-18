#ifndef Simulator_H
#define Simulator_H

#include <vector>

template<typename Parameters>
class Simulator {
	public:
		Simulator(unsigned int heapLength, unsigned int heapDepth, unsigned int reclaimSlope, bool fourDirectionsOnly = false);
		void clear();
		void stack(int position, const Parameters& parameters);
		bool reclaim(int& position, Parameters& parameters, std::vector<int>& heights);
		void resetReclaimer(void);

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
};

#include "Simulator.impl.h"

#endif
