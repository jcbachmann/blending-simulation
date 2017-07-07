#ifndef BlendingSimulatorDetailed_H
#define BlendingSimulatorDetailed_H

// STL
#include <list>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>

// Bullet
#include <bullet/LinearMath/btVector3.h>
#include <bullet/LinearMath/btQuaternion.h>

class btCollisionShape;

class btRigidBody;

class btDefaultMotionState;

class btBroadphaseInterface;

class btDefaultCollisionConfiguration;

class btCollisionDispatcher;

class btSequentialImpulseConstraintSolver;

class btCollisionShape;

class btDefaultMotionState;

class btRigidBody;

class btCollisionShape;

class btDefaultMotionState;

class btRigidBody;

class btHeightfieldTerrainShape;

class btDefaultMotionState;

class btRigidBody;

class btDiscreteDynamicsWorld;

// Local
#include "QualityCube.h"
#include "BlendingSimulator.h"

template<typename Parameters>
class ParticleLite;

template<typename Parameters>
class BlendingSimulatorDetailed : public BlendingSimulator<Parameters>
{
	public:
		std::map<int, std::map<int, QualityCube<Parameters>>> qualityGrid;
		std::mutex qualityGridMutex;

		BlendingSimulatorDetailed(float length, float depth);
		~BlendingSimulatorDetailed(void);

		virtual void clear(void) override;
		virtual void stack(double position, const Parameters& parameters) override;
		virtual void finish(void) override;
		virtual float* getHeapMap(void) override;

	protected:
		virtual std::vector<unsigned char> getRawData(void) override;
		virtual void setRawData(const std::vector<unsigned char>& data) override;

	private:
		float heapLength;
		float heapDepth;
		std::mutex simulationMutex;
		std::list<Particle<Parameters>*> activeParticles;
		std::atomic_bool activeParticlesAvailable;
		std::list<Particle<Parameters>*> allParticles;

		unsigned long long simulationTickCount;
		unsigned long long nextParticleTickCount;

		btBroadphaseInterface* broadphase;
		btDefaultCollisionConfiguration* collisionConfiguration;
		btCollisionDispatcher* dispatcher;
		btSequentialImpulseConstraintSolver* solver;
		btCollisionShape* groundShape;
		btDefaultMotionState* groundMotionState;
		btRigidBody* groundRigidBody;
		btHeightfieldTerrainShape* heapShape;
		btRigidBody* heapRigidBody;
		btDiscreteDynamicsWorld* dynamicsWorld;

		void step(void);
		void doOutputParticles(void);
		void freezeParticles(void);
		void freezeParticle(Particle<Parameters>* particle);
		Particle<Parameters>*
		createParticle(btVector3 position, Parameters parameters, bool frozen, btQuaternion rotation,
					   btVector3 velocity, btVector3 size);
};

#include "BlendingSimulatorDetailed.impl.h"

#endif