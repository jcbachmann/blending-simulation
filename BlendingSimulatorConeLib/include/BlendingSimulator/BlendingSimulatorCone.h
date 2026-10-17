#ifndef BlendingSimulatorCone_H
#define BlendingSimulatorCone_H

#include <vector>

#include "BlendingSimulator/BlendingSimulator.h"
#include "BlendingSimulator/ReclaimSlices.h"

namespace blendingsimulator
{
// The pile as the surface of a material at its angle of repose, without particles: every stacked portion (every call of stack, whatever
// its volume) forms a cone over the existing pile, and the new surface is the higher of the old surface and the cone,
//
//     h_new(x, z) = max(h_old(x, z), H - tan(angle of repose) * distance to the drop point),
//
// with the apex height H chosen so that the material between the two surfaces, the portion's shell, holds exactly its volume. This is the
// surface a critical-slope (angle of repose) sandpile reaches when material is poured at a point, the continuum limit of the particle
// rule of the fast simulation without its grid directions. Every grid column's part of the shell is reclaimed at the reclaim angle and
// spread over the slices it is cut into, like the particles of the other simulators.
//
// Deterministic, conserves the volume exactly, and changes continuously with the drop positions. The resolution is the cell size of the
// height grid, about 1 / particlesPerCubicMeter^(1/3) like the particle size of the other simulators (adjusted to tile the bed exactly).
// A stacker that moves while it stacks is represented by one call of stack per grid cell it crosses (the caller splits the material); a
// stacker standing still needs one call.
template<typename Parameters>
class BlendingSimulatorCone : public BlendingSimulator<Parameters>
{
	public:
		static constexpr double defaultAngleOfRepose = 45.0;

		explicit BlendingSimulatorCone(SimulationParameters simulationParameters, double angleOfRepose = defaultAngleOfRepose);

		void clear() override;
		void finishStacking() override;
		bool reclaimingFinished() override;
		Parameters reclaim(float position) override;

		// The whole volume of the parameters as one portion dropped at (x, z)
		void stack(float x, float z, const Parameters& parameters) override;

		// Edges of a grid cell in m: about 1 / ppm3^(1/3), adjusted so that the cells tile the bed exactly
		double getCellSizeX() const;
		double getCellSizeZ() const;

		// Height of the pile in m at the cell containing (x, z)
		double getHeight(double x, double z) const;

	protected:
		void stackSingle(float x, float z, const Parameters& parameters) override;
		void updateHeapMap() override;

	private:
		double cellX;
		double cellZ;
		int nx;
		int nz;

		// Slope of the cones, tan(angle of repose)
		double slope;

		// Reclaiming: tangent of the reclaim angle, vertical and horizontal faces, circumference of circular stockpiles
		double tanReclaimAngle;
		bool verticalReclaim;
		bool horizontalReclaim;
		double circumference = 0.0;

		// Pile height in m per cell, x-major: cell (i, j) at index i * nz + j
		std::vector<double> heights;

		// Cells that may receive material from the current portion and their base b = height + slope * distance to the drop point
		std::vector<double> candidateBase;
		std::vector<int> candidateCell;

		ReclaimSlices<Parameters> slices;

		void drop(double x, double z, const Parameters& parameters);

		// Collects the cells with b below the bound into the candidate lists
		void gather(double x, double z, double bound);

		// Apex height with exactly the given volume between the cone and the pile, starting from an upper bound
		double solveApex(double bound, double volume);

		// The part of the shell in cell (i, j) from height base with the given thickness into the reclaim slices
		void reclaimShell(int i, int j, double base, double thickness, const Parameters& parameters);
};
}

#include "detail/BlendingSimulatorCone.impl.h"

#endif
