#include <limits>

#include "Simulator.h"

template<typename Parameters>
Simulator<Parameters>::Simulator(unsigned int heapLength, unsigned int heapDepth, unsigned int reclaimSlope, bool fourDirectionsOnly)
		: heapLength(heapLength)
		, heapDepth(heapDepth)
		, reclaimSlope(reclaimSlope)
		, reclaimerPos(0)
		, fourDirectionsOnly(fourDirectionsOnly)
{
	stackedHeights.resize(heapLength + 2);
	for (unsigned int i = 0; i < heapLength + 2; i++) {
		stackedHeights[i].resize(heapDepth + 2);
	}

	reclaimParameters.resize(heapLength);

	clear();
}

template<typename Parameters>
void Simulator<Parameters>::clear()
{
	std::fill(stackedHeights[0].begin(), stackedHeights[0].end(), std::numeric_limits<int>::max());
	for (unsigned int i = 1; i < heapLength + 1; i++) {
		for (unsigned int j = 1; j < heapDepth + 1; j++) {
			stackedHeights[i][j] = 0;
		}
		stackedHeights[i][0] = std::numeric_limits<int>::max();
		stackedHeights[i][heapDepth + 1] = std::numeric_limits<int>::max();
	}
	std::fill(stackedHeights[heapLength + 1].begin(), stackedHeights[heapLength + 1].end(), std::numeric_limits<int>::max());

	for (unsigned int i = 0; i < heapLength; i++) {
		reclaimParameters[i].clear();
	}
}

template<typename Parameters>
void Simulator<Parameters>::stack(int position, const Parameters& parameters)
{
	const int depthCenter = heapDepth / 2;

	int i1 = position + 1;
	int j1 = depthCenter + 1;

	int minHeightI;
	int minHeightJ;
	int minHeight = stackedHeights[i1][j1];

	do {
		minHeightI = -1;
		minHeightJ = -1;

		struct Offset { int i; int j; };
		constexpr const static Offset offsets[] = {{-1,0},{0,-1},{0,+1},{+1,0},{-1,-1},{-1,+1},{+1,-1},{+1,+1}};
		const int offsetsCount = fourDirectionsOnly ? 4 : 8;

		for (int o = 0; o < offsetsCount; o++) {
			const int ti = i1 + offsets[o].i;
			const int tj = j1 + offsets[o].j;
			const int lh = stackedHeights[ti][tj];

			if (lh < minHeight) {
				minHeightI = ti;
				minHeightJ = tj;
				minHeight = lh;
			}
		}

		if (minHeightI >= 0) {
			i1 = minHeightI;
			j1 = minHeightJ;
		}
	} while (minHeightI >= 0);

	stackedHeights[i1][j1] = minHeight + 1;

	int reclaimIndex = i1 - (reclaimSlope > 0 ? minHeight / reclaimSlope : 0) - 1;

	if (reclaimIndex < 0) {
		reclaimIndex = 0;
	}

	if (reclaimIndex >= (int)heapLength) {
		reclaimIndex = heapLength - 1;
	}

	reclaimParameters[reclaimIndex].add(parameters);
}

template<typename Parameters>
bool Simulator<Parameters>::reclaim(int& position, Parameters& parameters, std::vector<int>& heights)
{
	if (reclaimerPos >= heapLength) {
		return false;
	}

	position = reclaimerPos;
	if (heights.size() != heapDepth) {
		heights.resize(heapDepth);
	}
	std::copy(stackedHeights[reclaimerPos + 1].begin() + 1, stackedHeights[reclaimerPos + 1].end() - 1, heights.begin());
	parameters = reclaimParameters[reclaimerPos];

	reclaimerPos++;
	return true;
}

template<typename Parameters>
void Simulator<Parameters>::resetReclaimer(void)
{
	reclaimerPos = 0;
}