#ifndef BlendingSimulatorLattice_H
#define BlendingSimulatorLattice_H

#include <cstdint>
#include <vector>

#include "BlendingSimulator/BlendingSimulator.h"
#include "BlendingSimulator/ReclaimSlices.h"

namespace blendingsimulator
{
// Particles on a hexagonal close-packed (HCP) lattice: layers of triangular lattices stacked ABAB. A particle falls until all three lattice
// sites below it are filled, preferring the site closest to the drop position when several are free.
//
// Lattice coordinates (xi, yi, zi) follow the hexsim proof of concept: (xi, yi - 1, zi) is always one of the three sites below, so filled
// sites form contiguous columns and one height per (xi, zi) describes the pile. The resting rule only depends on which sites are filled, so
// compressing the lattice vertically keeps every pile and scales all slopes: the compression maps nativeAngleOfRepose() of the uncompressed
// lattice to SimulationParameters::latticeAngleOfRepose. The particles become spheroids that keep their volume.
template<typename Parameters>
class BlendingSimulatorLattice : public BlendingSimulator<Parameters>
{
	public:
		// Angle of repose of the uncompressed lattice, as tangent and in degrees. A pile on the lattice is a hexagonal pyramid: every two
		// layers (2 sqrt(2/3) d higher) its level sets shrink by a regular hexagon of side d, the three sites below a site and the three
		// sites below the next layer combined. The cone of the same height and volume has the same base area, so its slope is
		// 2 sqrt(2/3) / sqrt(3 sqrt(3) / (2 pi)) = 4/3 sqrt(pi / sqrt(3)) = tan 60.89°. This is also the mean slope over all directions
		// (60.92°); the faces are steeper (tan = 4 sqrt(2) / 3, 62.06°) and the edges flatter (tan = 2 sqrt(2/3), 58.52°).
		static double nativeTanAngleOfRepose();
		static double nativeAngleOfRepose();

		explicit BlendingSimulatorLattice(SimulationParameters simulationParameters);

		void clear() override;
		void finishStacking() override;
		bool reclaimingFinished() override;
		Parameters reclaim(float position) override;

		// Horizontal particle diameter in m
		double getParticleDiameter() const;

		// Vertical particle size in m
		double getParticleHeight() const;

	protected:
		void stackSingle(float x, float z, const Parameters& parameters) override;
		void updateHeapMap() override;

	private:
		struct Site
		{
			int xi;
			int yi;
			int zi;
		};

		double verticalScale;
		double diameter; // Horizontal distance of neighbors in m
		double rowDistance; // Distance of rows in z in m
		double layerDistance; // Distance of layers in m

		// Column index ranges: xi in [minX, minX + columnsX), zi in [minZ, minZ + columnsZ)
		int minX;
		int minZ;
		int columnsX;
		int columnsZ;

		// Number of filled sites per column
		std::vector<int> heights;

		// Whether the site center of a column lies on the bed, per layer parity
		std::vector<std::uint8_t> valid[2];

		int maxHeight = 0;

		// Tangent of reclaim angle
		double tanReclaimAngle;

		// Particles grouped per cross section
		ReclaimSlices<Parameters> slices;

		static int parity(int i);

		// Column index or -1 outside of the column range
		int column(int xi, int zi) const;
		bool isValid(int xi, int yi, int zi) const;
		bool isFilled(int xi, int yi, int zi) const;
		void spacesBelow(const Site& site, Site below[3]) const;

		// Horizontal position in lattice units: x in diameters, z in rows
		void toUnits(const Site& site, double& xu, double& zu) const;
		Site fromUnits(double xu, double zu, int yi) const;

		// Resting site of a particle dropped at the given position in lattice units, false if it does not fit on the bed
		bool drop(double xu, double zu, Site& site) const;
};
}

#include "detail/BlendingSimulatorLattice.impl.h"

#endif
