#ifndef BlendingSimulator_H
#define BlendingSimulator_H

#include <deque>
#include <vector>
#include <mutex>
#include <map>
#include <atomic>

#include "Particle.h"
#include "ParameterCube.h"

template<typename Parameters>
class BlendingSimulator
{
	public:
		std::deque<Particle<Parameters>*> activeOutputParticles;
		std::deque<Particle<Parameters>*> inactiveOutputParticles;
		std::mutex outputParticlesMutex;
		std::mutex parameterCubesMutex;
		std::map<std::tuple<int, int, int>, ParameterCube<Parameters>*> parameterCubes;

		BlendingSimulator(float heapWorldSizeX, float heapWorldSizeZ, float reclaimAngle, float particlesPerCubicMeter, bool visualize);
		virtual ~BlendingSimulator();

		float* getHeapMap();
		std::pair<float, float> getHeapWorldSize();
		std::pair<unsigned int, unsigned int> getHeapMapSize();

		virtual void pause();
		virtual void resume();
		virtual bool isPaused();

		virtual void stack(float x, float z, const Parameters& parameters);

		virtual void clear() = 0;
		virtual void finishStacking() = 0;
		virtual bool reclaimingFinished() = 0;
		virtual Parameters reclaim(float position) = 0;

	protected:
		// Dimensions of the simulated stockpile
		const float heapWorldSizeX;
		const float heapWorldSizeZ;

		// Reclaimer angle measured in degree from 0 = horizontal to 90 = vertical
		const float reclaimAngle;

		unsigned int heapSizeX;
		unsigned int heapSizeZ;

		std::atomic<bool> paused;
		float* heapMap;
		const float particlesPerCubicMeter;
		const bool visualize;

		void initializeHeapMap(unsigned int heapSizeX, unsigned int heapSizeZ);

		virtual void stackSingle(float x, float z, const Parameters& parameters) = 0;

		virtual void updateHeapMap()
		{
		};
};

#include "BlendingSimulator.impl.h"

#endif
