#include <fstream>

template<typename Parameters>
BlendingSimulator<Parameters>::BlendingSimulator(SimulationParameters simulationParameters)
	: simulationParameters(simulationParameters)
	, heapSizeX(0)
	, heapSizeZ(0)
	, heapMap(nullptr)
	, paused(false)
{
}

template<typename Parameters>
BlendingSimulator<Parameters>::~BlendingSimulator()
{
	if (heapMap) {
		delete[] heapMap;
		heapMap = nullptr;
	}
}

template<typename Parameters>
float* BlendingSimulator<Parameters>::getHeapMap()
{
	updateHeapMap();

	return this->heapMap;
}

template<typename Parameters>
std::pair<float, float> BlendingSimulator<Parameters>::getHeapWorldSize()
{
	return {simulationParameters.heapWorldSizeX, simulationParameters.heapWorldSizeZ};
}

template<typename Parameters>
std::pair<unsigned int, unsigned int> BlendingSimulator<Parameters>::getHeapMapSize()
{
	return {heapSizeX, heapSizeZ};
}

template<typename Parameters>
void BlendingSimulator<Parameters>::pause()
{
	paused = true;
}

template<typename Parameters>
void BlendingSimulator<Parameters>::resume()
{
	paused = false;
}

template<typename Parameters>
bool BlendingSimulator<Parameters>::isPaused()
{
	return paused.load();
}

template<typename Parameters>
void BlendingSimulator<Parameters>::stack(float x, float z, const Parameters& parameters)
{
	float volumePerParticle = 1.0f / simulationParameters.particlesPerCubicMeter;
	parameterBuffer.push(parameters);

	while (parameterBuffer.contains(volumePerParticle)) {
		Parameters p = parameterBuffer.pop(volumePerParticle);
		this->stackSingle(x, z, p);
	}
}

template<typename Parameters>
void BlendingSimulator<Parameters>::initializeHeapMap(unsigned int pHeapSizeX, unsigned int pHeapSizeZ)
{
	if (heapMap) {
		throw std::runtime_error("invalid call to BlendingSimulator::initializeHeapMap(): heap map already initialized");
	}

	heapSizeX = pHeapSizeX;
	heapSizeZ = pHeapSizeZ;

	heapMap = new float[pHeapSizeZ * pHeapSizeX];
	for (int z = 0; z < pHeapSizeZ; z++) {
		for (int x = 0; x < pHeapSizeX; x++) {
			heapMap[z * pHeapSizeX + x] = 0.0f;
		}
	}
}
