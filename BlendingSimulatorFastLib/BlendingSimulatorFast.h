#ifndef BlendingSimulatorFast_H
#define BlendingSimulatorFast_H

#include <vector>

#include "BlendingSimulator.h"

template<typename Parameters>
class BlendingSimulatorFast : public BlendingSimulator<Parameters>
{
	public:
		explicit BlendingSimulatorFast(SimulationParameters simulationParameters);

		virtual void clear() override;
		virtual void finishStacking() override;
		virtual bool reclaimingFinished() override;
		virtual Parameters reclaim(float position) override;

	protected:
		virtual void stackSingle(float x, float z, const Parameters& parameters) override;
		virtual void updateHeapMap() override;

	private:
		// Size factor for calculating real world positions / sized from internal data
		const float realWorldSizeFactor;

		// Position up to which material has been reclaimed
		float reclaimerPos;

		// Tangent of reclaim angle
		float tanReclaimAngle;

		// Circumference of circular stockpile ridge
		double circumference = 0.0;

		// Variable tracking the height at each position for falling simulation
		std::vector<std::vector<int>> stackedHeights;

		// Variables for grouping the particles per cross section
		std::vector<Parameters> reclaimParameters;
};

#include "BlendingSimulatorFast.impl.h"

#endif
