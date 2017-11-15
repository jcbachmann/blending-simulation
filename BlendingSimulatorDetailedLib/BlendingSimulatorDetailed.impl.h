#include <random>
#include <thread>
#include <algorithm>

// Bullet
#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

// Local
#include "ParticleDetailed.h"

// Simulator constants
const float stackerDropOffAngle = btRadians(20); // Radians above horizon
const float stackerBeltSpeed = 3.0f; // In m/s
const float stackerBeltWidth = 1.5f; // In m
const int minFreezeTimeout = 15000;
const int maxFreezeTimeout = 25000;
const unsigned long long simulationInterval = 30;
const float cubicMetersPerSecond = 1.0; // in m³/s

template<typename Parameters>
BlendingSimulatorDetailed<Parameters>::BlendingSimulatorDetailed(float heapWorldSizeX, float heapWorldSizeZ, float reclaimAngle, float bulkDensityFactor,
	float particlesPerCubicMeter, float dropHeight, bool visualize)
	: BlendingSimulator<Parameters>(heapWorldSizeX, heapWorldSizeZ, reclaimAngle, particlesPerCubicMeter, visualize)
	, bulkDensityFactor(bulkDensityFactor)
	, particleSize(std::pow(bulkDensityFactor / particlesPerCubicMeter, 1.0 / 3.0))
	, resolutionPerWorldSize(1.0 / particleSize)
	, dropHeight(dropHeight)
	, simulationTicksPerParticle((unsigned long long) double(1000.0 * std::pow(particleSize, 3.0) / cubicMetersPerSecond))
	, simulationTickCount(0)
	, nextParticleTickCount(0)
	, activeParticlesAvailable(false)
{
	this->initializeHeapMap(int(heapWorldSizeX / particleSize + 0.5) + 1, int(heapWorldSizeZ / particleSize + 0.5) + 1);

	// Initialize physics
	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	broadphase = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver;
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0, -9.80665, 0));

	// Static ground shape
	groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 1);
	groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, -1, 0)));
	btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0));
	groundRigidBodyCI.m_friction = 10;
	groundRigidBody = new btRigidBody(groundRigidBodyCI);
	dynamicsWorld->addRigidBody(groundRigidBody);
}

template<typename Parameters>
BlendingSimulatorDetailed<Parameters>::~BlendingSimulatorDetailed()
{
	clear();

	std::lock_guard<std::mutex> lock(simulationMutex);

	// Ground
	dynamicsWorld->removeRigidBody(groundRigidBody);
	delete groundRigidBody->getMotionState();
	delete groundRigidBody;
	delete groundShape;

	// Physics
	delete dynamicsWorld;
	delete solver;
}

template<typename Parameters>
void BlendingSimulatorDetailed<Parameters>::clear()
{
	std::lock_guard<std::mutex> lock(simulationMutex);
	simulationTickCount = 0;

	for (int z = 0; z < this->heapSizeZ; z++) {
		for (int x = 0; x < this->heapSizeX; x++) {
			this->heapMap[z * this->heapSizeX + x] = 0;
		}
	}

	{
		std::lock_guard<std::mutex> innerLock(this->outputParticlesMutex);
		this->activeOutputParticles.clear();
		this->inactiveOutputParticles.clear();
	}

	activeParticles.clear();

	// TODO: This takes forever!
	while (!allParticles.empty()) {
		dynamicsWorld->removeRigidBody(allParticles.back()->rigidBody);

		delete allParticles.back();
		allParticles.pop_back();
	}
}

template<typename Parameters>
ParticleDetailed<Parameters>* BlendingSimulatorDetailed<Parameters>::createParticle(btVector3 position, Parameters parameters, bool frozen,
	btQuaternion rotation, btVector3 velocity, btVector3 size)
{
	ParticleDetailed<Parameters>* particle = new ParticleDetailed<Parameters>();

	particle->parameters = parameters;
	particle->frozen = frozen;
	particle->creationTickCount = simulationTickCount;
	particle->size = size;

	particle->collisionShape = new btBoxShape(0.5 * particle->size);
	particle->defaultMotionState = new btDefaultMotionState(btTransform(rotation, position));

	btScalar mass = size.x() * size.y() * size.z() / bulkDensityFactor;

	btVector3 fallInertia;

	particle->collisionShape->calculateLocalInertia(mass, fallInertia);

	btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass, particle->defaultMotionState, particle->collisionShape, fallInertia);

	fallRigidBodyCI.m_friction = 0.5;
	fallRigidBodyCI.m_linearDamping = 0.1;
	fallRigidBodyCI.m_angularDamping = 0.1;
	fallRigidBodyCI.m_restitution = 0;
	fallRigidBodyCI.m_additionalDamping = true;
	fallRigidBodyCI.m_additionalLinearDampingThresholdSqr = 0.5;
	fallRigidBodyCI.m_additionalAngularDampingThresholdSqr = 0.5;
	fallRigidBodyCI.m_additionalAngularDampingFactor = 0.5;
	fallRigidBodyCI.m_angularSleepingThreshold = 0.5;
	fallRigidBodyCI.m_linearSleepingThreshold = 0.5;

	particle->rigidBody = new btRigidBody(fallRigidBodyCI);
	particle->rigidBody->setDeactivationTime(0.05f);
	particle->rigidBody->setCcdMotionThreshold(0.5);

	allParticles.push_back(particle);
	activeParticles.push_back(particle);
	dynamicsWorld->addRigidBody(particle->rigidBody);

	if (particle->frozen) {
		freezeParticle(particle);
	} else {
		particle->rigidBody->setLinearVelocity(velocity);
	}

	return particle;
}

// Freeze old particles
template<typename Parameters>
void BlendingSimulatorDetailed<Parameters>::freezeParticles()
{
	for (auto it = activeParticles.begin(); it != activeParticles.end(); it++) {
		ParticleDetailed<Parameters>* particle = *it;

		if (particle->frozen) {
			continue;
		}

		if (simulationTickCount >= particle->creationTickCount && simulationTickCount - particle->creationTickCount >= maxFreezeTimeout ||
			particle->rigidBody->getActivationState() == WANTS_DEACTIVATION && simulationTickCount - particle->creationTickCount >= minFreezeTimeout) {
			freezeParticle(particle);
		}
	}
}

template<typename Parameters>
void BlendingSimulatorDetailed<Parameters>::freezeParticle(ParticleDetailed<Parameters>* particle)
{
	// Freeze position
	particle->rigidBody->setMassProps(btScalar(0), btVector3(0, 0, 0));
	particle->frozen = true;

	// Acquire position
	btTransform trans;
	particle->defaultMotionState->getWorldTransform(trans);
	const btVector3& origin = trans.getOrigin();

	// Update heap map
	addParticleToHeapMap(origin.x(), origin.y(), origin.z());

	// Looks nicer but totally ruins bulk density
//	addParticleToHeapMapBilinear(origin.x(), origin.y(), origin.z());
}

template<typename Parameters>
void BlendingSimulatorDetailed<Parameters>::addParticleToHeapMap(float x, float y, float z)
{
	// Scale position to heap map resolution
	x *= resolutionPerWorldSize;
	z *= resolutionPerWorldSize;

	// Calculate valid position indices
	int xi = std::max(0, std::min(int(x + 0.5f), int(this->heapSizeX) - 1));
	int zi = std::max(0, std::min(int(z + 0.5f), int(this->heapSizeZ) - 1));

	// Set heap map height to maximum of current value and y
	float& h = this->heapMap[zi * this->heapSizeX + xi];
	if (y > h) {
		h = y;
	}
}

void setBilinear(float* heapMap, int sizeX, int sizeZ, float x, float z, int xi, int zi, float vMin, float vMax)
{
	if (xi >= 0 & xi < sizeX && zi >= 0 && zi < sizeZ) {
		float dx = std::abs(float(xi) - x);
		float dz = std::abs(float(zi) - z);
		float v = vMin + (1.0f - dx) * (1.0f - dz) * (vMax - vMin);
		float& h = heapMap[zi * sizeX + xi];
		if (v > h) {
			h = v;
		}
	}
}

template<typename Parameters>
void BlendingSimulatorDetailed<Parameters>::addParticleToHeapMapBilinear(float x, float y, float z)
{
	// Scale position to heap map resolution
	x *= resolutionPerWorldSize;
	z *= resolutionPerWorldSize;

	float vMin = y - float(particleSize) * 0.5f;
	float vMax = y - float(particleSize) * 0.1f;
	setBilinear(this->heapMap, this->heapSizeX, this->heapSizeZ, x, z, int(x), int(z), vMin, vMax);
	setBilinear(this->heapMap, this->heapSizeX, this->heapSizeZ, x, z, int(x) + 1, int(z), vMin, vMax);
	setBilinear(this->heapMap, this->heapSizeX, this->heapSizeZ, x, z, int(x), int(z) + 1, vMin, vMax);
	setBilinear(this->heapMap, this->heapSizeX, this->heapSizeZ, x, z, int(x) + 1, int(z) + 1, vMin, vMax);
}

template<typename Parameters>
void BlendingSimulatorDetailed<Parameters>::optimizeFrozenParticles()
{
	for (auto it = allParticles.begin(); it != allParticles.end(); it++) {
		ParticleDetailed<Parameters>* particle = *it;

		if (particle->frozen && particle->inSimulation) {
			// Check if particle can be removed (below top layer)
			btTransform trans;
			particle->defaultMotionState->getWorldTransform(trans);
			btVector3& origin = trans.getOrigin();
			int x = int(origin.getX() + 0.5);
			int z = int(origin.getZ() + 0.5);

			if (x >= 0 & x < this->heapSizeX && z > 0 && z < this->heapSizeZ) {
				if (origin.getY() < this->heapMap[z * this->heapSizeX + x] - 4.0 * particleSize) {
					dynamicsWorld->removeRigidBody(particle->rigidBody);
					particle->inSimulation = false;
				}
			}
		}
	}
}

template<typename Parameters>
void BlendingSimulatorDetailed<Parameters>::finishStacking()
{
	while (activeParticlesAvailable.load()) {
		step();
	}
}

template<typename Parameters>
bool BlendingSimulatorDetailed<Parameters>::reclaimingFinished()
{
	return allParticles.empty();
}

template<typename Parameters>
Parameters BlendingSimulatorDetailed<Parameters>::reclaim(float position)
{
	float tanReclaimAngle;
	if (std::abs(90.0f - this->reclaimAngle) < 0.01) {
		tanReclaimAngle = 1e100;
	} else {
		tanReclaimAngle = std::tan(this->reclaimAngle * std::atan(1.0) * 4.0 / 180.0);
	}

	Parameters p;
	for (auto it = allParticles.begin(); it != allParticles.end();) {
		ParticleDetailed<Parameters>* particle = *it;

		btTransform trans;
		particle->defaultMotionState->getWorldTransform(trans);
		btVector3& origin = trans.getOrigin();

		float comparePosition = origin.x();
		if (tanReclaimAngle > 1e10) {
			// Vertical
		} else if (tanReclaimAngle < 1e-10) {
			// Horizontal
			if (this->reclaimAngle < 90.0f) {
				comparePosition = 0;
			} else {
				comparePosition = this->heapWorldSizeX;
			}
		} else {
			comparePosition -= origin.y() / tanReclaimAngle;
		}

		if (comparePosition < position) {
			p.push(particle->parameters);
			dynamicsWorld->removeRigidBody(particle->rigidBody);
			delete particle;
			it = allParticles.erase(it);
		} else {
			it++;
		}
	}

	return p;
}

template<typename Parameters>
void BlendingSimulatorDetailed<Parameters>::stackSingle(float x, float z, const Parameters& parameters)
{
	while (simulationTickCount < nextParticleTickCount) {
		// TODO wait for particle parameter time
		step();
	}

	static const float sizeVariation = float(particleSize) * 0.05f;
	static const float positionVariation = 0.5f * stackerBeltWidth;
	static const float miscVariation = 0.005f; // 1 +/- variation for speed, height, and angle

	static std::random_device rd;
	static std::default_random_engine generator(rd());
	static std::uniform_real_distribution<float> sizeDist(-sizeVariation, sizeVariation);
	static std::uniform_real_distribution<float> posDist(-positionVariation, positionVariation);
	static std::uniform_real_distribution<float> minVarDist(1 - miscVariation, 1 + miscVariation);
	static std::uniform_real_distribution<float> angle(0.0f, 2.0f * 3.141592653589793238463f);

	createParticle(
		btVector3(
			x + posDist(generator),
			dropHeight * minVarDist(generator),
			z - 5.0f
		), // Position
		parameters, // Parameters
		false, // Frozen
		btQuaternion(btVector3(0, 0, 1), angle(generator)), // Orientation
		btVector3(0, 0, 1).rotate(btVector3(-1, 0, 0), stackerDropOffAngle * minVarDist(generator)) * stackerBeltSpeed *
			minVarDist(generator), // Angle and speed
		btVector3(float(particleSize) + sizeDist(generator), float(particleSize) + sizeDist(generator), float(particleSize) + sizeDist(generator)) // Size
	);

	nextParticleTickCount = simulationTickCount + simulationTicksPerParticle;
}

template<typename Parameters>
void BlendingSimulatorDetailed<Parameters>::step()
{
	if (this->paused.load()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		return;
	}

	std::lock_guard<std::mutex> lock(simulationMutex);

	const int subSteps = 3;
	const float timeStep = float(simulationInterval) / 1000.0f;
	dynamicsWorld->stepSimulation(timeStep, subSteps, timeStep / float(subSteps));
	doOutputParticles();
	freezeParticles();
	static int optimizeFrozenParticlesCounter = 0;
	optimizeFrozenParticlesCounter = (optimizeFrozenParticlesCounter + 1) % 100;
	if (optimizeFrozenParticlesCounter == 0) {
		optimizeFrozenParticles();
	}
	simulationTickCount += simulationInterval;
}

std::tuple<double, double, double, double> toTuple(btQuaternion q)
{
	return std::make_tuple(q.w(), q.x(), q.y(), q.z());
};

std::tuple<double, double, double> toTuple(btVector3 v)
{
	return std::make_tuple(v.x(), v.y(), v.z());
};

// Output particle details to graphics interface
template<typename Parameters>
void BlendingSimulatorDetailed<Parameters>::doOutputParticles()
{
	std::lock_guard<std::mutex> lock(this->outputParticlesMutex);

	for (auto it = activeParticles.begin(); it != activeParticles.end();) {
		ParticleDetailed<Parameters>* particle = *it;

		if (!particle->outputParticle) {
			particle->outputParticle = new Particle<Parameters>();

			this->activeOutputParticles.push_back(particle->outputParticle);
		}

		btTransform trans;
		particle->defaultMotionState->getWorldTransform(trans);

		particle->outputParticle->parameters = particle->parameters;
		particle->outputParticle->frozen = particle->frozen;
		particle->outputParticle->temperature =
			std::max(0.0f, std::min(1.0f - float(simulationTickCount - particle->creationTickCount) / float(minFreezeTimeout), 1.0f));
		particle->outputParticle->position = bs::Vector3(toTuple(trans.getOrigin()));
		particle->outputParticle->size = bs::Vector3(toTuple(particle->size));
		particle->outputParticle->orientation = bs::Quaternion(toTuple(trans.getRotation()));

		if (particle->frozen) {
			it = activeParticles.erase(it);
			this->activeOutputParticles.erase(std::find(this->activeOutputParticles.begin(), this->activeOutputParticles.end(), particle->outputParticle));
			this->inactiveOutputParticles.push_back(particle->outputParticle);
		} else {
			it++;
		}
	}

	activeParticlesAvailable = !activeParticles.empty();
}
