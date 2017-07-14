#ifndef BLENDINGSIMULATOR_PARAMETERCUBE_H
#define BLENDINGSIMULATOR_PARAMETERCUBE_H

#include <deque>

#include "BasicTypes.h"
#include "ParticleParameters.h"

template<typename Parameters>
class ParameterCube
{
	public:
		ParameterCube(bs::Vector3 position, float size)
			: position(position)
			, size(size)
		{
		}

		bs::Vector3 getPosition(void) const
		{
			return position;
		}

		float getSize(void) const
		{
			return size;
		}

		Parameters getParameters(void) const
		{
			return parameters;
		}

		void add(const Parameters& particleParameters)
		{
			parameters.add(particleParameters);
		}

	protected:
		bs::Vector3 position;
		float size;
		Parameters parameters;
};

#endif
