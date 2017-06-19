#include "QualityCube.h"

#include "Particle.h"

template<typename Parameters>
QualityCube<Parameters>::QualityCube(void)
	: initialized(false)
{
}

template<typename Parameters>
void QualityCube<Parameters>::initialize(btVector3 position, btVector3 size)
{
	this->position = position;
	this->size = size;

	initialized = true;
}

template<typename Parameters>
void QualityCube<Parameters>::add(Particle<Parameters>* particle)
{
	particles.push_back(particle);

	parameters.add(particle->parameters);
}

template<typename Parameters>
Parameters QualityCube<Parameters>::getAverage() const
{
	Parameters parameters;
	for (Particle<Parameters>* particle : particles) {
		parameters.add(particle->parameters);
	}
	return parameters;
}
