#ifndef BLENDINGSIMULATOR_PARTICLE_H
#define BLENDINGSIMULATOR_PARTICLE_H

#include "detail/BasicTypes.h"

namespace blendingsimulator
{
template<typename Parameters>
struct Particle
{
	Parameters parameters;
	bool frozen;

	Vector3 position;
	Vector3 size;
	Quaternion orientation;

	Particle()
		: frozen(false)
	{
	}
};
}

#endif
