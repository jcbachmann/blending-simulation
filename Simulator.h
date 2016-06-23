#ifndef Simulator_H
#define Simulator_H

#include <vector>

class Simulator {
	public:
		Simulator(unsigned int heapLength, unsigned int heapDepth, unsigned reclaimSlope);
		void clear();
		void stack(int position, int red, int blue, int yellow);
		bool reclaim(int& p, int& r, int& b, int& y, std::vector<int>& heights);

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
		std::vector<int> reclaimRed;
		std::vector<int> reclaimBlue;
		std::vector<int> reclaimYellow;
};


#endif
