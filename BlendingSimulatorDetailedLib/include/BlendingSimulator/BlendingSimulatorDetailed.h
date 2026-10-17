#ifndef BlendingSimulatorDetailed_H
#define BlendingSimulatorDetailed_H

#include <memory>
#include <vector>

#include "BlendingSimulator/BlendingSimulator.h"
#include "BlendingSimulator/ReclaimSlices.h"
#include "BlendingSimulator/dem/World.h"

namespace blendingsimulator
{
template<typename Parameters>
struct Particle;

// Detailed simulation with the discrete element method (dem::World): every particle is a sphere that falls from the stacker's material
// stream, collides, slides and rolls with Hertz-Mindlin contacts and rolling resistance, and comes to rest on the pile. Particles stand for
// parcels of 1/particlesPerCubicMeter m³ of bulk material; their radii follow from DetailedParameters::packingFraction so that the
// settled pile has the stacked volume.
//
// The stacker releases the material of each stack() call at the flow rate of DetailedParameters::flowRate: a particle leaves the boom
// dischargeHeight above the pile surface at the stacker position, with beltSpeed along +z, aimed so that the stream lands at the stacker
// position. Reclaiming happens after stacking from the final particle positions: a reclaimer face at reclaimAngle moving along x (or around
// the circle of a circular stockpile), each particle spread over the slices it cuts like in the other simulators.
template<typename Parameters>
class BlendingSimulatorDetailed : public BlendingSimulator<Parameters>
{
	public:
		explicit BlendingSimulatorDetailed(SimulationParameters simulationParameters);
		~BlendingSimulatorDetailed() override;

		void clear() override;
		void finishStacking() override;
		bool reclaimingFinished() override;
		Parameters reclaim(float position) override;

		// The physics world, for diagnostics
		const dem::World& getWorld() const { return *world; }

		// Bulk volume in m³ of the particles that left the bed area and are not reclaimed
		double getLostVolume() const;

		// Mean particle radius in m
		double getMeanRadius() const { return meanRadius; }

		// Simulated seconds the discharge was blocked (all release points taken), by which stacking took longer than the flow rate gives
		double getBlockedTime() const { return blockedTime; }

	protected:
		void stackSingle(float x, float z, const Parameters& parameters) override;
		void updateHeapMap() override;

	private:
		// Seconds of simulated time after the last release within which the pile must come to rest in finishStacking
		static constexpr double maxSettleTime = 120.0;
		// Interval in simulated seconds of the particle output for a visualizer
		static constexpr double outputInterval = 0.1;

		double particleVolume; // Bulk volume per particle in m³
		double meanRadius; // Mean particle radius in m
		double cellSize; // Heap map cell in m

		// The stream at the discharge (see the constructor): a lattice of release points in a horizontal plane, streamColumns across the belt
		// (x) by streamRows along it (z), which the material crosses downward at streamDropSpeed after falling streamDrop from the head pulley
		int streamColumns = 1;
		int streamRows = 1;
		double streamCellWidth = 0.0; // in x, in m
		double streamSpacing = 0.0; // in z, and the least distance of two release points, in m
		double streamDrop = 0.0; // in m
		double streamDropSpeed = 0.0; // in m/s
		double streamClearTime = 0.0; // in s, after which a particle released at a point has left it

		std::unique_ptr<dem::World> world;
		std::vector<Parameters> particleParameters; // By particle index of the world
		double releaseTime = 0.0;
		double blockedTime = 0.0;
		double nextOutput = 0.0;

		ReclaimSlices<Parameters> slices;
		bool reclaimPrepared = false;

		std::vector<Particle<Parameters>*> outputParticles; // By particle index, only when visualizing

		void createWorld();
		void advanceTo(double t);
		void prepareReclaim();
		void updateOutputParticles(bool final);
		void deleteOutputParticles();
};
}

#include "detail/BlendingSimulatorDetailed.impl.h"

#endif
