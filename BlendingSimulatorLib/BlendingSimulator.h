#ifndef BlendingSimulator_H
#define BlendingSimulator_H

#include <list>
#include <vector>
#include <mutex>

#include "ParticleLite.h"

template<typename Parameters>
class BlendingSimulator
{
	public:
		std::list<ParticleLite<Parameters>*> outputParticles;
		std::list<ParticleLite<Parameters>*> activeOutputParticles;
		std::mutex outputParticlesMutex;
		int heapMapRes;

		BlendingSimulator(void)
			: paused(false)
		{
		}

		virtual void pause(void)
		{
			paused = true;
		}

		virtual void resume(void)
		{
			paused = false;
		}

		virtual bool isPaused(void)
		{
			return paused.load();
		}

		virtual void clear(void) = 0;
		virtual void stack(double position, const Parameters& parameters) = 0;
		virtual void finish(void) = 0;
		virtual float* getHeapMap(void) = 0;

		void loadFromFile(std::string filename);
		void saveToFile(std::string filename);

	protected:
		std::atomic<bool> paused;
		float* heapMap;

		virtual std::vector<unsigned char> getRawData(void) = 0;
		virtual void setRawData(const std::vector<unsigned char>& data) = 0;
};

#include "BlendingSimulator.impl.h"

#endif
