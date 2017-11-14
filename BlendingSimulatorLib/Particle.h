#ifndef BLENDINGSIMULATOR_PARTICLE_H
#define BLENDINGSIMULATOR_PARTICLE_H

#include "BasicTypes.h"

template<typename Parameters>
struct Particle
{
	Parameters parameters;
	bool frozen;
	float temperature;

	bs::Vector3 position;
	bs::Vector3 size;
	bs::Quaternion orientation;

	Particle()
		: frozen(false)
	{
	}
};

#endif
