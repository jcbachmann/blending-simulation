#ifndef BLENDINGSIMULATOR_SYSTEMPARAMETERS_H
#define BLENDINGSIMULATOR_SYSTEMPARAMETERS_H

namespace blendingsimulator
{
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

	/// Sacrifice some speed to provide visualization output
	bool visualize = false;


	/* Detailed simulation */

	/// Typical bulk density of cube shaped particles in detailed simulation used for volume correction
	float bulkDensityFactor = 1.0f;

	/// Height in m above ground from which particles are dropped
	float dropHeight = 10.0f;
};
}

#endif
