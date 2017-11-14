#include <random>
#include <thread>

// Bullet
#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

// Local
#include "ParticleDetailed.h"

// Simulator constants
const float stackerDropOffHeight = 28.0f; // In m above ground
const float stackerDropOffAngle = btRadians(20); // Radians above horizon
const float stackerBeltSpeed = 3.0f; // In m/s
const float stackerBeltWidth = 1.5f; // In m
const int minFreezeTimeout = 15000;
const int maxFreezeTimeout = 25000;
const float parameterCubeSize = 1.8f;
const float heapMaxHeight = stackerDropOffHeight - 1.0f;
const unsigned long long simulationInterval = 30;
const double averageParticleSize = 0.5; // m
const double cubicMetersPerSecond = 0.36; // m³/s
const unsigned long long simulationTicksPerParticle = (unsigned long long) (1000.0 * pow(averageParticleSize, 3.0) /
																			cubicMetersPerSecond);

template<typename Parameters>
BlendingSimulatorDetailed<Parameters>::BlendingSimulatorDetailed(float length, float depth)
	: BlendingSimulator<Parameters>()
	, heapLength(length)
	, heapDepth(depth)
	, simulationTickCount(0)
	, nextParticleTickCount(0)
{
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

	// Dynamic heap shape
	this->heapMapRes = int(std::pow(2, std::ceil(std::log(std::max(heapLength, heapDepth)) / std::log(2))) + 0.5) + 1;
	this->heapMap = new float[this->heapMapRes * this->heapMapRes];
	for (int i = 0; i < this->heapMapRes; i++) {
		for (int j = 0; j < this->heapMapRes; j++) {
			this->heapMap[i * this->heapMapRes + j] = 0.0f;
		}
	}

	heapShape = new btHeightfieldTerrainShape(
		this->heapMapRes, // int heightStickWidth
		this->heapMapRes, // int heightStickLength
		reinterpret_cast<void*>(this->heapMap), // const void* heightfieldData
		1.0f, // btScalar heightScale
		0.0, // btScalar minHeight
		heapMaxHeight, // btScalar maxHeight
		1, // int upAxis
		PHY_FLOAT, // PHY_ScalarType heightDataType
		false // bool flipQuadEdges
	);
	heapShape->setLocalScaling(btVector3(1.0, 1.0, 1.0));
	btTransform tr;
	tr.setIdentity();
	tr.setOrigin(btVector3((this->heapMapRes - 1) / 2.0f, heapMaxHeight / 2.0f, (this->heapMapRes - 1) / 2.0f));
	btRigidBody::btRigidBodyConstructionInfo heapRigidBodyCI(0, new btDefaultMotionState(tr), heapShape, btVector3(0, 0, 0));
	heapRigidBodyCI.m_friction = 1;
	heapRigidBody = new btRigidBody(heapRigidBodyCI);
	dynamicsWorld->addRigidBody(heapRigidBody);
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

	// Heap
	delete[] this->heapMap;
	dynamicsWorld->removeRigidBody(heapRigidBody);
	delete heapRigidBody->getMotionState();
	delete heapRigidBody;
	delete heapShape;

	// Physics
	delete dynamicsWorld;
	delete solver;
}

template<typename Parameters>
void BlendingSimulatorDetailed<Parameters>::clear()
{
	std::lock_guard<std::mutex> lock(simulationMutex);
	simulationTickCount = 0;

	for (int i = 0; i < this->heapMapRes; i++) {
		for (int j = 0; j < this->heapMapRes; j++) {
			this->heapMap[i * this->heapMapRes + j] = 0;
		}
	}

	{
		std::lock_guard<std::mutex> innerLock(this->outputParticlesMutex);
		this->outputParticles.clear();
		this->activeOutputParticles.clear();
	}

	{
		std::lock_guard<std::mutex> innerLock(this->parameterCubesMutex);
		this->parameterCubes.clear();
	}

	activeParticles.clear();

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

	btScalar mass = 1;

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

		if (simulationTickCount >= particle->creationTickCount && simulationTickCount - particle->creationTickCount >= maxFreezeTimeout) {
			freezeParticle(particle);
			continue;
		}

		if (particle->rigidBody->getActivationState() == WANTS_DEACTIVATION && simulationTickCount - particle->creationTickCount >= minFreezeTimeout) {
			freezeParticle(particle);
		}
	}
}

template<typename Parameters>
void BlendingSimulatorDetailed<Parameters>::freezeParticle(ParticleDetailed<Parameters>* particle)
{
	dynamicsWorld->removeRigidBody(particle->rigidBody);
	particle->frozen = true;

	// Update heap
	btTransform trans;
	particle->defaultMotionState->getWorldTransform(trans);
	btVector3& origin = trans.getOrigin();
	int x = int(origin.getX() + 0.5);
	int z = int(origin.getZ() + 0.5);

	if (x > 0 & x < this->heapMapRes && z > 0 && z < this->heapMapRes) {
		this->heapMap[z * this->heapMapRes + x] = std::max(this->heapMap[z * this->heapMapRes + x], origin.getY() - float(averageParticleSize) * 0.1f);
	}

	{
		std::tuple<int, int, int> id = std::make_tuple(
			int(origin.getX() / parameterCubeSize),
			int(origin.getY() / parameterCubeSize),
			int(origin.getZ() / parameterCubeSize)
		);

		ParameterCube<Parameters>* parameterCube = nullptr;
		std::lock_guard<std::mutex> lock(this->parameterCubesMutex);
		auto it = this->parameterCubes.find(id);
		if (it != this->parameterCubes.end()) {
			parameterCube = it->second;
		} else {
			parameterCube = new ParameterCube<Parameters>(
				bs::Vector3((float(std::get<0>(id)) + 0.5f) * parameterCubeSize,
							(float(std::get<1>(id)) + 0.5f) * parameterCubeSize,
							(float(std::get<2>(id)) + 0.5f) * parameterCubeSize),
				parameterCubeSize
			);
			this->parameterCubes[id] = parameterCube;
		}

		parameterCube->add(particle->parameters);
	}
}

template<typename Parameters>
void BlendingSimulatorDetailed<Parameters>::stack(double position, const Parameters& parameters)
{
	while (simulationTickCount < nextParticleTickCount) {
		step();
	}

	static const float sizeVariation = float(averageParticleSize) * 0.05f;
	static const float positionVariation = 0.5f * stackerBeltWidth;
	static const float miscVariation = 0.005f; // 1 +/- variation for speed, height, and angle

	static std::default_random_engine generator;
	static std::uniform_real_distribution<float> sizeDist(-sizeVariation, sizeVariation);
	static std::uniform_real_distribution<float> posDist(-positionVariation, positionVariation);
	static std::uniform_real_distribution<float> minVarDist(1 - miscVariation, 1 + miscVariation);

	createParticle(
		btVector3(float(position) + posDist(generator), stackerDropOffHeight * minVarDist(generator), heapDepth / 2.0f - 5.0f), // Position
		parameters, // Parameters
		false, // Frozen
		btQuaternion(btVector3(1, 0, 0), 0), // Orientation
		btVector3(0, 0, 1).rotate(btVector3(-1, 0, 0), stackerDropOffAngle * minVarDist(generator)) * stackerBeltSpeed * minVarDist(generator), // Angle and speed
		btVector3(float(averageParticleSize) + sizeDist(generator), float(averageParticleSize) + sizeDist(generator), float(averageParticleSize) + sizeDist(generator)) // Size
	);

	nextParticleTickCount = simulationTickCount + simulationTicksPerParticle;
}

template<typename Parameters>
void BlendingSimulatorDetailed<Parameters>::finish(void)
{
	while (activeParticlesAvailable.load()) {
		step();
	}
}

template<typename Parameters>
float* BlendingSimulatorDetailed<Parameters>::getHeapMap(void)
{
	return this->heapMap;
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

			this->outputParticles.push_back(particle->outputParticle);
			this->activeOutputParticles.push_back(particle->outputParticle);
		}

		btTransform trans;
		particle->defaultMotionState->getWorldTransform(trans);

		particle->outputParticle->parameters = particle->parameters;
		particle->outputParticle->frozen = particle->frozen;
		particle->outputParticle->temperature = std::max(0.0f, std::min(1.0f - float(simulationTickCount - particle->creationTickCount) / float(minFreezeTimeout), 1.0f));
		particle->outputParticle->position = bs::Vector3(toTuple(trans.getOrigin()));
		particle->outputParticle->size = bs::Vector3(toTuple(particle->size));
		particle->outputParticle->orientation = bs::Quaternion(toTuple(trans.getRotation()));

		if (particle->frozen) {
			it = activeParticles.erase(it);
			this->activeOutputParticles.erase(std::find(this->activeOutputParticles.begin(), this->activeOutputParticles.end(), particle->outputParticle));
		} else {
			it++;
		}
	}

	activeParticlesAvailable = !activeParticles.empty();
}


// TODO implement reclaiming for detailed simulation
// Old code for reclaiming
/*
	const int lines = HEAP_LENGTH;

	std::array<int, lines> amounts;
	std::array<float, lines> heights;
	std::array<float, lines> qualities;

	for (int i = 0; i < lines; i++) {
		amounts[i] = 0;
		heights[i] = 0;
		qualities[i] = 0;
	}

	{
		std::lock_guard <std::mutex> lock(simulator->outputParticlesMutex);

		for (std::deque<Particle*>::iterator it = simulator->outputParticles.begin();
			 it != simulator->outputParticles.end(); it++) {
			Particle* p = *it;

			int line = std::min(
				std::max(int(float(lines) * (p->position.x() - p->position.z()) / float(HEAP_LENGTH) + 0.5), 0),
				lines - 1);

			if (p->position.z() > heights[line]) {
				heights[line] = p->position.z();
			}

			amounts[line] = amounts[line] + 1;
			qualities[line] = qualities[line] + p->quality;
		}
	}

	FILE* outputFile = fopen(".\\Output.txt", "w");

	if (outputFile) {
		for (int i = 0; i < lines; i++) {
			std::string output = "Amount:\t" + std::to_string(amounts[i]);
			output += "\tHeight:\t" + std::to_string(heights[i]);
			output += "\tQuality:\t" + std::to_string(amounts[i] > 0 ? qualities[i] / float(amounts[i]) : 0);
			output += "\n";

			fwrite(output.c_str(), 1, output.size(), outputFile);
		}

		fclose(outputFile);
	} else {
		Log::e("Ouput file could not be opened.");
	}
 */