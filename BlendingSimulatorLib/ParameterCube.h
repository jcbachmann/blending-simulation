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

		bs::Vector3 getPosition() const
		{
			return position;
		}

		float getSize() const
		{
			return size;
		}

		Parameters getParameters() const
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
