#ifndef BLENDINGSIMULATOR_SYSTEMPARAMETERS_H
#define BLENDINGSIMULATOR_SYSTEMPARAMETERS_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

namespace blendingsimulator
{
// Parameters of the detailed simulation: the stacker, the particles and their material. The material defaults describe a free-flowing,
// coarse bulk material such as lump ore with an angle of repose of about 45°, the convention of the other simulators; the README of
// BlendingSimulatorDetailedLib documents the calibration (rollingFriction 0.1 gives about 36°, 0.2 about 39°, as typical for coal and
// fine ore).
struct DetailedParameters
{
	/* Stacker */

	/// Bulk volume flow of the stacker in m³/s; it sets the simulated time per stacked volume (1 m³/s is about 1,500 to 2,500 t/h)
	double flowRate = 1.0;

	/// Height in m of the discharge above the pile surface below it: the boom luffs with the growing pile
	double dischargeHeight = 1.5;

	/// Horizontal speed in m/s at which the material leaves the boom's discharge, along +z; the stream is aimed so that it lands at the
	/// stacker position. Discharge chutes and deflector hoods slow the belt speed of 2 to 5 m/s down to about 1 m/s
	double beltSpeed = 1.0;

	/// Width in m of the material stream across the boom (in x)
	double beltWidth = 1.5;

	/* Particles */

	/// Solid volume fraction of the settled pile: particles are sized so that the pile takes up the stacked bulk volume
	double packingFraction = 0.6;

	/// Relative spread of the particle radii: uniform within +/- sizeSpread around the mean (a few percent avoid crystalline packings)
	double sizeSpread = 0.1;

	/* Material */

	double density = 2600.0; ///< Density of the particle solid in kg/m³
	double youngsModulus = 2.0e8; ///< In Pa, reduced as is common in DEM, so that particles overlap by about 1 to 2 % under a 10 m pile
	double poissonRatio = 0.25;
	double restitution = 0.3;
	double friction = 0.6; ///< Sliding friction between particles and with the ground
	double rollingFriction = 0.4; ///< Rolling resistance (EPSD) between particles and with the ground; sets the angle of repose
	double rollingDamping = 0.3; ///< Damping ratio of the rolling resistance

	/* Numerics */

	/// Time step as a fraction of the Rayleigh time of the smallest particle. Under the weight of a pile the Hertz contacts stiffen and
	/// spheres with a dozen contacts turn stiffer than they translate: 0.25 is unstable already, 0.15 keeps a margin
	double timeStepFactor = 0.15;

	/// Pairs are kept for particles closer than their radii plus skinFactor times the largest radius (a larger skin searches less often
	/// but keeps more pairs)
	double skinFactor = 0.4;

	/// A particle slower than restVelocity in m/s for restTime in s falls asleep until a moving particle touches it
	double restVelocity = 0.05;
	double restTime = 0.5;

	/// Threads of the force computation, 0 for all cores; the result does not depend on it
	int threads = 1;
};

/// Eight likelihood of the fast simulation fitted to the detailed simulation (discrete element method, 0.25 to 0.33 m spheres, calibrated
/// to 45 degrees) on chevron, optimized and random depositions of a 59 m by 20 m bed: by level k = 1 + log4(particlesPerCubicMeter)
/// (1, 4, 16, 64, 256, 1,024 particles per cubic meter are the levels 1 to 6) 0.60, 0.60, 0.70, 0.75, 0.80, 0.85, linear between
/// levels and constant outside them. With these values the fast simulation agrees with the detailed one about equally well at every
/// level from 16 particles per cubic meter up, where 0.87 was right only at 1,024
inline float fittedEightLikelihood(float particlesPerCubicMeter)
{
	const float values[6] = {0.60f, 0.60f, 0.70f, 0.75f, 0.80f, 0.85f};
	const double level = 1.0 + std::log(std::max(double(particlesPerCubicMeter), 1e-9)) / std::log(4.0);
	if (level <= 1.0) {
		return values[0];
	}
	if (level >= 6.0) {
		return values[5];
	}
	const int lower = int(std::floor(level));
	const double weight = level - lower;
	return float(values[lower - 1] * (1.0 - weight) + values[lower] * weight);
}

struct SimulationParameters
{
	/* Generic */

	/// Length of the blending bed in m
	float heapWorldSizeX = 100.0f;

	/// Depth of the blending bed in m
	float heapWorldSizeZ = 20.0f;

	/// Angle at which the virtual reclaimer collects material
	float reclaimAngle = 45.0f;

	/// Degree of detail measuring the amount of particles one cubic meter of material should be split up into
	float particlesPerCubicMeter = 1.0f;

	/// Simulate a circular stockpile with the stacker moving from the center of the world
	bool circular = false;


	/* Fast simulation */

	/// Likelihood of considering 8 instead of 4 fall directions resulting in cone like shapes instead of pyramids. Negative (the default
	/// since 2026.4): the likelihood fitted to the detailed simulation for the particle size, fittedEightLikelihood(particlesPerCubicMeter);
	/// releases up to 2026.3 used 0.87 at every particle size
	float eightLikelihood = -1.0f;

	/// If positive, replaces eightLikelihood by a likelihood that grows with the height h (in particles) of the cell a particle is dropped
	/// on: max(0, 1 - eightHeightScale / h). A constant likelihood is tried at every step of the fall, and a particle takes more steps on
	/// finer grids and larger piles, so its piles steepen with particlesPerCubicMeter and pile size; with this scaling the expected number
	/// of 4-direction steps per particle stays the same, and 6.5 gives round piles at 45 degrees from 16 particles per cubic meter up,
	/// independent of the resolution and the pile size
	float eightHeightScale = 0.0f;

	/// Sacrifice some speed to provide visualization output
	bool visualize = false;


	/* Detailed simulation (discrete element method, see BlendingSimulatorDetailed) */

	/// Unused since 2026.3: the pile's packing is a result of the discrete element method now, particles are sized with
	/// DetailedParameters::packingFraction
	float bulkDensityFactor = 1.0f;

	/// Unused since 2026.3: the stacker discharges at DetailedParameters::dischargeHeight above the pile
	float dropHeight = 10.0f;

	DetailedParameters detailed;


	/* Reproducibility */

	/// Seed of the random number generator, a simulator without seed draws a random one so that its results cannot be repeated
	std::optional<std::uint32_t> seed;
};
}

#endif
