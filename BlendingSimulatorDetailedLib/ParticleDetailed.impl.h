#include "ParticleDetailed.h"

// Bullet
#include <btBulletDynamicsCommon.h>

// Blending Simulator Lib
#include "Particle.h"

template<typename Parameters>
ParticleDetailed<Parameters>::ParticleDetailed()
	: frozen(false)
	, inSimulation(true)
	, creationTickCount(0)
	, collisionShape(nullptr)
	, rigidBody(nullptr)
	, defaultMotionState(nullptr)
	, outputParticle(nullptr)
{
}

template<typename Parameters>
ParticleDetailed<Parameters>::~ParticleDetailed()
{
	if (collisionShape) {
		delete collisionShape;
	}

	if (rigidBody) {
		delete rigidBody;
	}

	if (defaultMotionState) {
		delete defaultMotionState;
	}

	if (outputParticle) {
		delete outputParticle;
	}
}
