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
		BlendingSimulatorDetailed(float heapWorldSizeX, float heapWorldSizeZ, float reclaimAngle, float bulkDensityFactor, float particlesPerCubicMeter,
			float dropHeight, bool visualize);
		~BlendingSimulatorDetailed();

		virtual void clear() override;
		virtual void finishStacking() override;
		virtual bool reclaimingFinished() override;
		virtual Parameters reclaim(float position) override;

	protected:
		void stackSingle(float x, float z, const Parameters& parameters) override;

	private:
		std::mutex simulationMutex;
		std::deque<ParticleDetailed<Parameters>*> activeParticles;
		std::atomic_bool activeParticlesAvailable;
		std::deque<ParticleDetailed<Parameters>*> allParticles;

		const float dropHeight; // In m above ground
		const float bulkDensityFactor;
		const float particleSize; // In m cube side length
		const float resolutionPerWorldSize; // Cells per meter

		const unsigned long long simulationTicksPerParticle;
		unsigned long long simulationTickCount;
		unsigned long long nextParticleTickCount;

		btBroadphaseInterface* broadphase;
		btDefaultCollisionConfiguration* collisionConfiguration;
		btCollisionDispatcher* dispatcher;
		btSequentialImpulseConstraintSolver* solver;
		btCollisionShape* groundShape;
		btDefaultMotionState* groundMotionState;
		btRigidBody* groundRigidBody;
		btDiscreteDynamicsWorld* dynamicsWorld;

		void step();
		void doOutputParticles();
		void freezeParticles();
		void freezeParticle(ParticleDetailed<Parameters>* particle);
		void addParticleToHeapMap(float x, float y, float z);
		void addParticleToHeapMapBilinear(float x, float y, float z);
		void optimizeFrozenParticles();
		ParticleDetailed<Parameters>* createParticle(btVector3 position, Parameters parameters, bool frozen, btQuaternion rotation, btVector3 velocity,
			btVector3 size);
};

#include "BlendingSimulatorDetailed.impl.h"

#endif
