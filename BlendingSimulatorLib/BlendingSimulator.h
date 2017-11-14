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

		void loadFromFile(std::string filename);
		void saveToFile(std::string filename);

		virtual void clear() = 0;
	protected:
		std::atomic<bool> paused;
		float* heapMap;

		virtual std::vector<unsigned char> getRawData(void) = 0;
		virtual void setRawData(const std::vector<unsigned char>& data) = 0;
};

#include "BlendingSimulator.impl.h"

#endif
