#ifndef BlendingSimulatorFast_H
#define BlendingSimulatorFast_H

#include <cstdint>
#include <vector>

#include "BlendingSimulator/BlendingSimulator.h"
#include "BlendingSimulator/ReclaimSlices.h"

namespace blendingsimulator
{
template<typename Parameters>
class BlendingSimulatorFast : public BlendingSimulator<Parameters>
{
	public:
		explicit BlendingSimulatorFast(SimulationParameters simulationParameters);

		void clear() override;
		void finishStacking() override;
		bool reclaimingFinished() override;
		Parameters reclaim(float position) override;

	protected:
		void stackSingle(float x, float z, const Parameters& parameters) override;
		void updateHeapMap() override;

	private:
		// Size factor for calculating real world positions / sized from internal data
		const float realWorldSizeFactor;

		// Tangent of reclaim angle
		float tanReclaimAngle;

		// Circumference of circular stockpile ridge
		double circumference = 0.0;

		// Height in particles at each position for falling simulation, x-major with a border of one cell that particles cannot fall into:
		// cell (x, z) of the bed is at index (x + 1) * rowLength + z + 1
		std::vector<int> stackedHeights;

		// Number of cells along z including the border
		int rowLength;

		// Index differences of the neighbors in the order of offsets: 4 direct neighbors first, then the 4 diagonals
		int neighborIndexOffsets[8];

		// Particles grouped per cross section
		ReclaimSlices<Parameters> slices;

		// A fall step considers 8 instead of 4 directions if the upper 29 bits of its random number are below this threshold
		std::uint32_t eightThreshold;

		// Index of the neighbor a particle falls to from cell `index` at height `height`, or -1 if it rests there
		int fallStep(int index, int height, std::uint32_t random) const;
};
}

#include "detail/BlendingSimulatorFast.impl.h"

#endif
