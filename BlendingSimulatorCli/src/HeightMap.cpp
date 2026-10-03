#include "HeightMap.h"

void writeHeightMap(std::ostream& out, blendingsimulator::BlendingSimulator<blendingsimulator::AveragedParameters>& simulator)
{
	auto heapMapSize = simulator.getHeapMapSize();
	const float* heapMap = simulator.getHeapMap();
	for (unsigned int z = 0; z < heapMapSize.second; z++) {
		for (unsigned int x = 0; x < heapMapSize.first; x++) {
			if (x > 0) {
				out << "\t";
			}
			out << heapMap[z * heapMapSize.first + x];
		}
		out << "\n";
	}
}
