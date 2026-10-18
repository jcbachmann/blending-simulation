#ifndef BLENDINGSIMULATOR_SYSTEMPARAMETERS_H
#define BLENDINGSIMULATOR_SYSTEMPARAMETERS_H

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

	/// Likelihood of considering 8 instead of 4 fall directions resulting in cone like shapes instead of pyramids
	float eightLikelihood = 0.87f;

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
