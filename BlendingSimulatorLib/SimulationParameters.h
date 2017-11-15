#ifndef BLENDINGSIMULATOR_SYSTEMPARAMETERS_H
#define BLENDINGSIMULATOR_SYSTEMPARAMETERS_H

struct SimulationParameters
{
	/* Generic */

	/// Length of the blending bed in m
	float heapWorldSizeX = 0.0f;

	/// Depth of the blending bed in m
	float heapWorldSizeZ = 0.0f;

	/// Angle at which the virtual reclaimer collects material
	float reclaimAngle = 0.0f;

	/// Degree of detail measuring the amount of particles one cubic meter of material should be split up into
	float particlesPerCubicMeter = 0.0f;


	/* Fast simulation */

	/// Likelihood of considering 8 instead of 4 fall directions resulting in cone like shapes instead of pyramids
	float eightLikelihood = 0.0f;

	/// Sacrifice some speed to provide visualization output
	bool visualize = false;


	/* Detailed simulation */

	/// Typical bulk density of cube shaped particles in detailed simulation used for volume correction
	float bulkDensityFactor = 0.0f;

	/// Height in m above ground from which particles are dropped
	float dropHeight = 0.0f;
};

#endif
