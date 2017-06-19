#ifndef QualityCubeH
#define QualityCubeH

// STL
#include <list>

// Bullet
#include <bullet/LinearMath/btVector3.h>

// Local
template<typename Parameters>
class Particle;

template<typename Parameters>
struct QualityCube
{
	bool initialized;
	std::list<Particle<Parameters>*> particles;

	Parameters parameters;
	btVector3 position;
	btVector3 size;

	QualityCube(void);
	void initialize(btVector3 position, btVector3 size);
	void add(Particle<Parameters>* particle);
	Parameters getAverage(void) const;
};

#include "QualityCube.impl.h"

#endif
