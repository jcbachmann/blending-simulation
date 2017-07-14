#ifndef BlendingSimulatorDetailed_H
#define BlendingSimulatorDetailed_H

// STL
#include <deque>
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
#include "BlendingSimulator.h"

template<typename Parameters>
class Particle;

template<typename Parameters>
struct ParticleDetailed;

template<typename Parameters>
class BlendingSimulatorDetailed : public BlendingSimulator<Parameters>
{
	public:
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
		std::deque<ParticleDetailed<Parameters>*> activeParticles;
		std::atomic_bool activeParticlesAvailable;
		std::deque<ParticleDetailed<Parameters>*> allParticles;

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
		void freezeParticle(ParticleDetailed<Parameters>* particle);
		ParticleDetailed<Parameters>* createParticle(btVector3 position, Parameters parameters, bool frozen, btQuaternion rotation, btVector3 velocity, btVector3 size);
};

#include "BlendingSimulatorDetailed.impl.h"

#endif
