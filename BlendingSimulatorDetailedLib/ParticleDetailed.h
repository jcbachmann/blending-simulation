#ifndef ParticleDetailedH
#define ParticleDetailedH

// Bullet
#include <bullet/LinearMath/btVector3.h>
#include <bullet/LinearMath/btQuaternion.h>

class btCollisionShape;

class btRigidBody;

class btDefaultMotionState;

// Blending Simulator Lib
template<typename Parameters>
struct Particle;

template<typename Parameters>
struct ParticleDetailed
{
	btVector3 size;

	bool frozen;
	unsigned long long creationTickCount;

	Parameters parameters;
	btCollisionShape* collisionShape;
	btRigidBody* rigidBody;
	btDefaultMotionState* defaultMotionState;

	Particle<Parameters>* outputParticle;

	ParticleDetailed(void);
	~ParticleDetailed(void);
};

#include "ParticleDetailed.impl.h"

#endif
