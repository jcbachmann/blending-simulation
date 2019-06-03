#ifndef BLENDINGSIMULATOR_PARTICLE_H
#define BLENDINGSIMULATOR_PARTICLE_H

#include "detail/BasicTypes.h"

template<typename Parameters>
struct Particle
{
	Parameters parameters;
	bool frozen;

	bs::Vector3 position;
	bs::Vector3 size;
	bs::Quaternion orientation;

	Particle()
		: frozen(false)
	{
	}
};

#endif
