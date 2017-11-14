#ifndef BlendingSimulator_H
#define BlendingSimulator_H

#include <deque>
#include <vector>
#include <mutex>
#include <map>

#include "Particle.h"
#include "ParameterCube.h"

template<typename Parameters>
class BlendingSimulator
{
	public:
		std::deque<Particle<Parameters>*> outputParticles;
		std::deque<Particle<Parameters>*> activeOutputParticles;
		std::mutex outputParticlesMutex;
		int heapMapRes;
		std::mutex parameterCubesMutex;
		std::map<std::tuple<int, int, int>, ParameterCube<Parameters>*> parameterCubes;

		BlendingSimulator(void);


		virtual void stack(double position, const Parameters& parameters) = 0;
		virtual void finish(void) = 0;
		virtual float* getHeapMap(void) = 0;
		virtual void pause();
		virtual void resume();
		virtual bool isPaused();


		virtual void clear() = 0;
	protected:
		std::atomic<bool> paused;
		float* heapMap;

};

#include "BlendingSimulator.impl.h"

#endif
