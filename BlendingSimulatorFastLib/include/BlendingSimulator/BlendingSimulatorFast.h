#ifndef BlendingSimulatorFast_H
#define BlendingSimulatorFast_H

#include <random>
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

		// Variable tracking the height at each position for falling simulation
		std::vector<std::vector<int>> stackedHeights;

		// Particles grouped per cross section
		ReclaimSlices<Parameters> slices;

		// Decides between 4 and 8 fall directions by comparison with eightLikelihood
		std::uniform_real_distribution<double> coneDistribution{0.0, 1.0};

		// Rotates the order in which the fall directions are checked
		std::uniform_int_distribution<int> directionDistribution{0, 7};
};
}

#include "detail/BlendingSimulatorFast.impl.h"

#endif
