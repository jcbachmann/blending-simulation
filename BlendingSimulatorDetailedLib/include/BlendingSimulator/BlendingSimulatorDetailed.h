#ifndef BlendingSimulatorDetailed_H
#define BlendingSimulatorDetailed_H

// STL
#include <deque>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>

// Bullet
#include <LinearMath/btVector3.h>
#include <LinearMath/btQuaternion.h>

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

class btDefaultMotionState;

class btRigidBody;

class btDiscreteDynamicsWorld;

// Local
#include "BlendingSimulator/BlendingSimulator.h"

namespace blendingsimulator
{
template<typename Parameters>
class Particle;

template<typename Parameters>
struct ParticleDetailed;

template<typename Parameters>
class BlendingSimulatorDetailed : public BlendingSimulator<Parameters>
{
	public:
		explicit BlendingSimulatorDetailed(SimulationParameters simulationParameters);
		~BlendingSimulatorDetailed();

		void clear() override;
		void finishStacking() override;
		bool reclaimingFinished() override;
		Parameters reclaim(float position) override;

	protected:
		void stackSingle(float x, float z, const Parameters& parameters) override;

	private:
		// System constants
		static constexpr const float stackerDropOffAngle = 20 * BlendingSimulator<Parameters>::pi / 180.0; // Radians above horizon
		static constexpr const float stackerBeltSpeed = 3.0f; // In m/s
		static constexpr const float stackerBeltWidth = 1.5f; // In m
		static constexpr const float cubicMetersPerSecond = 1.0; // in m³/s

		// Simulator constants
		static const int minFreezeTimeout = 15000;
		static const int maxFreezeTimeout = 25000;
		static const unsigned long long simulationIntervalMs = 30;
		static const int simulationIntervalSubSteps = 3;

		std::mutex simulationMutex;
		std::deque<ParticleDetailed<Parameters>*> activeParticles;
		std::atomic_bool activeParticlesAvailable;
		std::deque<ParticleDetailed<Parameters>*> allParticles;

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
}

#include "detail/BlendingSimulatorDetailed.impl.h"

#endif
