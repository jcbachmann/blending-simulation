#ifndef ParticleH
#define ParticleH

// Bullet
#include <bullet/LinearMath/btVector3.h>
#include <bullet/LinearMath/btQuaternion.h>

class btCollisionShape;

class btRigidBody;

class btDefaultMotionState;

// Blending Simulator Lib
template<typename Parameters>
struct ParticleLite;

template<typename Parameters>
struct Particle
{
	btVector3 size;

	bool frozen;
	unsigned long long creationTickCount;

	Parameters parameters;
	btCollisionShape* collisionShape;
	btRigidBody* rigidBody;
	btDefaultMotionState* defaultMotionState;

	ParticleLite<Parameters>* outputParticle;

	Particle(void);
	~Particle(void);
};

#include "Particle.impl.h"

#endif
