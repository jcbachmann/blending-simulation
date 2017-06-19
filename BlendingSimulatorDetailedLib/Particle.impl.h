#include "Particle.h"

// Bullet
#include <bullet/btBulletDynamicsCommon.h>

// Blending Simulator Lib
#include "ParticleLite.h"

template<typename Parameters>
Particle<Parameters>::Particle(void)
	: frozen(false)
	, creationTickCount(0)
	, collisionShape(nullptr)
	, rigidBody(nullptr)
	, defaultMotionState(nullptr)
	, outputParticle(nullptr)
{
}

template<typename Parameters>
Particle<Parameters>::~Particle(void)
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
