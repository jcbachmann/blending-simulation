#include <fstream>

template<typename Parameters>
BlendingSimulator<Parameters>::BlendingSimulator(float heapWorldSizeX, float heapWorldSizeZ, float reclaimAngle, float particlesPerCubicMeter, bool visualize)
	: heapWorldSizeX(heapWorldSizeX)
	, heapWorldSizeZ(heapWorldSizeZ)
	, reclaimAngle(std::max(0.0f, std::min(reclaimAngle, 180.0f)))
	, particlesPerCubicMeter(particlesPerCubicMeter)
	, visualize(visualize)
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
	return {heapWorldSizeX, heapWorldSizeZ};
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
	float volumePerParticle = 1.0f / particlesPerCubicMeter;
	static Parameters parameterBuffer;
	parameterBuffer.push(parameters);

	while (parameterBuffer.contains(volumePerParticle)) {
		Parameters p = parameterBuffer.pop(volumePerParticle);
		this->stackSingle(x, z, p);
	}
}

template<typename Parameters>
void BlendingSimulator<Parameters>::initializeHeapMap(unsigned int heapSizeX, unsigned int heapSizeZ)
{
	if (heapMap) {
		throw std::runtime_error("invalid call to BlendingSimulator::initializeHeapMap(): heap map already initialized");
	}

	this->heapSizeX = heapSizeX;
	this->heapSizeZ = heapSizeZ;

	heapMap = new float[heapSizeZ * heapSizeX];
	for (int z = 0; z < heapSizeZ; z++) {
		for (int x = 0; x < heapSizeX; x++) {
			heapMap[z * heapSizeX + x] = 0.0f;
		}
	}
}