#ifndef BlendingSimulatorFormula_H
#define BlendingSimulatorFormula_H

#include <vector>

#include "BlendingSimulator/BlendingSimulator.h"
#include "BlendingSimulator/ReclaimSlices.h"

namespace blendingsimulator
{
// The cone simulation (BlendingSimulatorCone) reduced exactly to the stacker line, without a grid across the bed or in height.
//
// Every stacked portion forms a cone at the angle of repose over the pile, h_new = max(h_old, H - k * distance to the drop point) with
// k = tan(angle of repose), the apex H solved so that the shell holds the portion's volume. When all drops lie on one line z = z0 along the
// bed, the pile's level set at height y is a union of disks centred on that line, so the half-width of the cross-section at x is
//
//     w(x, y) = max_j sqrt((H_j - y)^2 - m_j^2) / k,   m_j = k |x - x_j|,
//
// the upper envelope of the hyperbolic sections of the drops' cones. The squared widths of two sections differ by a linear function of y,
// so two sections cross at most once: each cross-section is a short list of pieces (H_j, m_j) on height intervals, and the shell of a new
// cone, its derivative in H and the new envelope follow in closed form (integrals of sqrt(s^2 - m^2)). The volume is integrated along the
// bed per cell (midpoint rule over cells of about one particle size, adjusted to tile the bed exactly, like the cone simulation); the walls
// of the bed at z = 0 and z = depth clamp the widths. The apex is solved with Newton's method on the convex, increasing shell volume.
//
// Material at height y is reclaimed at x - y / tan(reclaim angle). The shell of a cell is reclaimed per piece of the envelope it covers, in
// one height band or in two if the piece spans more than 1 m, each band uniform in height and spread over the cell. These bands are coarser
// than the columns of the cone simulation: at the same cell size the reclaimed slice volumes of both differ by about 1 % (rms), F1 and the
// spread of the slice volumes by less than 0.3 %. Deterministic, conserves the volume exactly, changes continuously with the drop positions.
//
// Limits: all drops must lie on one line along the bed (constant z; stack throws std::invalid_argument for a drop off the line), and
// circular beds are not supported (the constructor throws). A moving stacker needs one stack call per cell it crosses (ContinuousDrops.h).
template<typename Parameters>
class BlendingSimulatorFormula : public BlendingSimulator<Parameters>
{
	public:
		static constexpr double defaultAngleOfRepose = 45.0;

		explicit BlendingSimulatorFormula(SimulationParameters simulationParameters, double angleOfRepose = defaultAngleOfRepose);

		void clear() override;
		void finishStacking() override;
		bool reclaimingFinished() override;
		Parameters reclaim(float position) override;

		// The whole volume of the parameters as one portion dropped at (x, z); z must be the z of all earlier drops since the last clear
		void stack(float x, float z, const Parameters& parameters) override;

		// Edge of a cell along the bed in m: about 1 / ppm3^(1/3), adjusted so that the cells tile the bed exactly
		double getCellSizeX() const;

		// Height of the pile in m at (x, z), x taken at the center of its cell: the highest cone of the cell's envelope at the distance of z
		// from the stacker line
		double getHeight(double x, double z) const;

		// Apex height of the last drop in m
		double getLastApex() const;

	protected:
		void stackSingle(float x, float z, const Parameters& parameters) override;
		void updateHeapMap() override;

	private:
		// A drop's cone section in one cell, apex H and m = k * distance along the bed, dominating the cross-section on [ya, yb]
		struct Piece
		{
			double H;
			double m;
			double ya;
			double yb;
		};

		// Pieces kept per cell; beyond that the shortest is merged into its neighbour (the envelope gets slightly narrower there)
		static constexpr int maxPieces = 24;

		// Height intervals longer than this many m are reclaimed in two bands
		static constexpr double bandHeight = 1.0;

		// Relative volume tolerance of the apex
		static constexpr double tolerance = 1e-7;

		double cellX;
		int nx;

		// Cells of the height map across the bed (only for getHeapMap)
		double cellZ;
		int nz;

		// tan(angle of repose)
		double slope;

		// Reclaiming: tangent of the reclaim angle and its inverse, vertical and horizontal faces
		double tanReclaimAngle;
		double inverseTanReclaimAngle;
		bool verticalReclaim;
		bool horizontalReclaim;

		// The stacker line and the distances of the walls from it, set by the first drop
		bool lineSet = false;
		double lineZ = 0.0;
		double wall1 = 0.0;
		double wall2 = 0.0;

		// Per cell: the envelope (maxPieces slots, pieces sorted by height) and the height of the pile on the stacker line
		std::vector<Piece> pieces;
		std::vector<int> pieceCount;
		std::vector<double> ridge;

		// Shell cross-section of the current drop per cell
		std::vector<double> shell;

		// The previous drop, for the start of the apex search
		double lastX = -1e9;
		double lastVolume = 0.0;
		double lastExcess = 0.0;
		double lastApex = 0.0;

		ReclaimSlices<Parameters> slices;

		void drop(double x, double z, const Parameters& parameters);

		// Integral over [ya, yb] of min(c, sqrt((H - y)^2 - m^2) / k), the section ending at height top, and its derivative in H
		double sectionIntegral(double H, double m, double top, double c, double ya, double yb, double* dH) const;

		// Shell of the cone (H, m) in cell i, both sides, and its derivative in H; visit(ya, yb, Hj, mj, old, area) gets every part
		template<typename Visit>
		double cellShell(int i, double H, double m, double* dH, Visit&& visit) const;

		// Adds the cone (H, m) to the envelope of cell i
		void updateEnvelope(int i, double H, double m);

		// Material of the cone (H, m) in cell i between ya and yb above the piece (Hj, mj) (none if !old), both sides
		double band(double H, double m, double Hj, double mj, bool old, double ya, double yb) const;

		void reclaimBand(double x, double ya, double yb, const Parameters& parameters, double volume);
};
}

#include "detail/BlendingSimulatorFormula.impl.h"

#endif
