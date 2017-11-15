#ifndef BlendingSimulator_H
#define BlendingSimulator_H

#include <deque>
#include <vector>
#include <mutex>
#include <map>
#include <atomic>

#include "Particle.h"
#include "SimulationParameters.h"

template<typename Parameters>
class BlendingSimulator
{
	public:
		std::deque<Particle<Parameters>*> activeOutputParticles;
		std::deque<Particle<Parameters>*> inactiveOutputParticles;
		std::mutex outputParticlesMutex;

		explicit BlendingSimulator(SimulationParameters simulationParameters);
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
		SimulationParameters simulationParameters;

		std::atomic<bool> paused;

		unsigned int heapSizeX;
		unsigned int heapSizeZ;
		float* heapMap;

		void initializeHeapMap(unsigned int heapSizeX, unsigned int heapSizeZ);

		virtual void stackSingle(float x, float z, const Parameters& parameters) = 0;

		virtual void updateHeapMap()
		{
		};
};

#include "BlendingSimulator.impl.h"

#endif
