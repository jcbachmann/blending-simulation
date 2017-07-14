#ifndef BLENDINGSIMULATOR_PARTICLELITE_H
#define BLENDINGSIMULATOR_PARTICLELITE_H

#include "BasicTypes.h"

template<typename Parameters>
struct ParticleLite
{
	Parameters parameters;
	bool frozen;
	float temperature;

	bs::Vector3 position;
	bs::Vector3 size;
	bs::Quaternion orientation;

	ParticleLite(void)
		: frozen(false)
	{
	}
};

#endif
