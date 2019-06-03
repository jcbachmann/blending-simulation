#ifndef ParticleDetailedH
#define ParticleDetailedH

// Bullet
#include <LinearMath/btVector3.h>
#include <LinearMath/btQuaternion.h>

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
	bool inSimulation;
	unsigned long long creationTickCount;

	Parameters parameters;
	btCollisionShape* collisionShape;
	btRigidBody* rigidBody;
	btDefaultMotionState* defaultMotionState;

	Particle<Parameters>* outputParticle;

	ParticleDetailed();
	~ParticleDetailed();
};

#endif

#include "ParticleDetailed.impl.h"
