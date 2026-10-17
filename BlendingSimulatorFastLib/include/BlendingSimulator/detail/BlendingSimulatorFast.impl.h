#include <algorithm>
#include <cmath>
#include <limits>
#include <thread>

template<typename Parameters>
blendingsimulator::BlendingSimulatorFast<Parameters>::BlendingSimulatorFast(SimulationParameters simulationParameters)
	: BlendingSimulator<Parameters>(simulationParameters)
	, realWorldSizeFactor(1.0f / std::pow(simulationParameters.particlesPerCubicMeter, 1.0f / 3.0f))
{
	if (std::abs(90.0f - simulationParameters.reclaimAngle) < 0.01) {
		tanReclaimAngle = 1e100;
	} else {
		tanReclaimAngle = float(std::tan(simulationParameters.reclaimAngle * std::atan(1.0) * 4.0 / 180.0));
	}

	this->initializeHeapMap(
		(unsigned int)(simulationParameters.heapWorldSizeX / realWorldSizeFactor + 0.5),
		(unsigned int)(simulationParameters.heapWorldSizeZ / realWorldSizeFactor + 0.5)
	);

	// Warning: stackedHeights organized first x than z while heap map is first z than x!
	rowLength = int(this->heapSizeZ) + 2;
	stackedHeights.resize(std::size_t(this->heapSizeX + 2) * rowLength);

	// Same order as the fall directions of the original implementation: {-1, 0}, {0, -1}, {0, +1}, {+1, 0}, then the diagonals
	// {-1, -1}, {-1, +1}, {+1, -1}, {+1, +1}, as {x, z}
	const int offsets[8][2] = {{-1, 0}, {0, -1}, {0, +1}, {+1, 0}, {-1, -1}, {-1, +1}, {+1, -1}, {+1, +1}};
	for (int o = 0; o < 8; o++) {
		neighborIndexOffsets[o] = offsets[o][0] * rowLength + offsets[o][1];
	}

	// Upper 29 bits of a 32 bit random number below the threshold: probability eightLikelihood
	const double eight = std::max(0.0, std::min(double(simulationParameters.eightLikelihood), 1.0));
	eightThreshold = std::uint32_t(eight * double(1u << 29));

	if (simulationParameters.circular) {
		circumference = 2.0 * this->pi * 0.25 * std::min(simulationParameters.heapWorldSizeX, simulationParameters.heapWorldSizeZ);
		slices.resize(static_cast<unsigned int>(circumference / realWorldSizeFactor + 0.5), realWorldSizeFactor);
	} else {
		slices.resize(this->heapSizeX, realWorldSizeFactor);
	}

	clear();
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorFast<Parameters>::clear()
{
	// Border cells are infinitely high so that particles never fall into them
	std::fill(stackedHeights.begin(), stackedHeights.end(), std::numeric_limits<int>::max());
	for (unsigned int x = 1; x < this->heapSizeX + 1; x++) {
		std::fill_n(stackedHeights.begin() + std::size_t(x) * rowLength + 1, this->heapSizeZ, 0);
	}

	slices.clear();

	{
		std::lock_guard<std::mutex> lock(this->outputParticlesMutex);

		for (auto particle : this->activeOutputParticles) {
			delete particle;
		}
		this->activeOutputParticles.clear();

		for (auto particle : this->inactiveOutputParticles) {
			delete particle;
		}
		this->inactiveOutputParticles.clear();
	}
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorFast<Parameters>::finishStacking()
{
	// The rest of the buffer, less than one particle, would be lost otherwise
	this->stackRest();
}

template<typename Parameters>
bool blendingsimulator::BlendingSimulatorFast<Parameters>::reclaimingFinished()
{
	return slices.finished();
}

template<typename Parameters>
Parameters blendingsimulator::BlendingSimulatorFast<Parameters>::reclaim(float position)
{
	return slices.reclaim(position);
}

template<typename Parameters>
int blendingsimulator::BlendingSimulatorFast<Parameters>::fallStep(int index, int height, std::uint32_t random) const
{
	// 8 fall directions with probability eightLikelihood, else the 4 direct neighbors only: this results in cones instead of pyramids
	const int count = (random >> 3) < eightThreshold ? 8 : 4;

	// The neighbors are searched in the order of neighborIndexOffsets rotated by a random amount; the first of the lowest wins. A plain
	// loop with a branch is fastest here: the processor predicts the branch well enough to start on the next step early, while branchless
	// and vectorized (AVX2) versions of this search took 1.6 to 2.8 times as long.
	const int rotation = int(random & 7u);
	int lowest = height;
	int next = -1;
	for (int o = 0; o < count; o++) {
		const int neighbor = index + neighborIndexOffsets[(o + rotation) & (count - 1)];
		const int neighborHeight = stackedHeights[neighbor];
		if (neighborHeight < lowest) {
			next = neighbor;
			lowest = neighborHeight;
		}
	}
	return next;
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorFast<Parameters>::stackSingle(float x, float z, const Parameters& parameters)
{
	while (this->paused.load()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	// Cell i (stackedHeights index i + 1 because of the border) covers the positions from i to i + 1 particle sizes
	int xi = std::max(0, std::min(static_cast<int>(std::floor(x / realWorldSizeFactor)), int(this->heapSizeX - 1))) + 1;
	int zi = std::max(0, std::min(static_cast<int>(std::floor(z / realWorldSizeFactor)), int(this->heapSizeZ - 1))) + 1;
	int index = xi * rowLength + zi;

	int minHeight = stackedHeights[index];

	// TODO replace variable slow by single step button on interface
	bool slow = false;

	static const std::chrono::milliseconds simulationSleep(300);

	Particle<Parameters>* particle = nullptr;

	if (this->simulationParameters.visualize) {
		particle = new Particle<Parameters>();
		particle->parameters = parameters;
		particle->frozen = false;
		particle->size = Vector3(0.95 * realWorldSizeFactor, 0.95 * realWorldSizeFactor, 0.95 * realWorldSizeFactor);
		particle->orientation = Quaternion(0, 0, 1, 0);

		if (slow) {
			particle->position = Vector3(
				(float(xi) - 0.5f) * realWorldSizeFactor,
				(float(minHeight) + 0.5f) * realWorldSizeFactor,
				(float(zi) - 0.5f) * realWorldSizeFactor
			);

			{
				std::lock_guard<std::mutex> lock(this->outputParticlesMutex);
				this->activeOutputParticles.push_back(particle);
			}

			std::this_thread::sleep_for(simulationSleep);
		}
	}

	// Simulate particle falling, one random number per step
	while (true) {
		const int next = fallStep(index, minHeight, std::uint32_t(this->randomEngine() >> 32));
		if (next < 0) {
			break;
		}
		index = next;
		minHeight = stackedHeights[index];

		if (this->simulationParameters.visualize && slow) {
			{
				std::lock_guard<std::mutex> lock(this->outputParticlesMutex);
				particle->position = Vector3(
					(float(index / rowLength) - 0.5f) * realWorldSizeFactor,
					(float(minHeight) + 0.5f) * realWorldSizeFactor,
					(float(index % rowLength) - 0.5f) * realWorldSizeFactor
				);
			}

			std::this_thread::sleep_for(simulationSleep);
		}
	}
	xi = index / rowLength;
	zi = index % rowLength;

	// Update height
	stackedHeights[index] = minHeight + 1;

	if (this->simulationParameters.visualize) {
		{
			std::lock_guard<std::mutex> lock(this->outputParticlesMutex);

			if (slow) {
				this->activeOutputParticles.pop_back();
			}

			particle->position = Vector3(
				(float(xi) - 0.5f) * realWorldSizeFactor,
				(float(minHeight) + 0.5f) * realWorldSizeFactor,
				(float(zi) - 0.5f) * realWorldSizeFactor
			);
			particle->frozen = true;
			this->inactiveOutputParticles.push_back(particle);
		}

		if (slow) {
			std::this_thread::sleep_for(simulationSleep);
		}
	}

	// Prepare reclaiming: the particle center is reclaimed at its horizontal position minus its height divided by tan(reclaim angle), and
	// the particle is spread over the slices the reclaimer cuts it in
	const double size = realWorldSizeFactor;
	const double centerX = (double(xi) - 0.5) * size;
	const double centerZ = (double(zi) - 0.5) * size;
	const double centerY = (double(minHeight) + 0.5) * size;
	const bool vertical = tanReclaimAngle > 1e10;
	const bool horizontal = tanReclaimAngle < 1e-10;
	const double depth = vertical || horizontal ? 0.0 : size / std::abs(double(tanReclaimAngle));

	if (this->simulationParameters.circular) {
		// Position along the circumference, from the angle around the bed center
		const double dx = centerX - 0.5 * this->simulationParameters.heapWorldSizeX;
		const double dz = centerZ - 0.5 * this->simulationParameters.heapWorldSizeZ;
		double posOnCircumference = 0.5 * (1.0 - std::atan2(dz, dx) / this->pi) * this->circumference;

		// Horizontal reclaiming is ignored for circular stockpiles - it is useless anyway
		if (!vertical && !horizontal) {
			posOnCircumference -= centerY / tanReclaimAngle;
		}

		slices.push(posOnCircumference, size, depth, parameters, true);
	} else if (horizontal) {
		slices.push(this->simulationParameters.reclaimAngle < 90.0f ? 0 : static_cast<int>(slices.size() - 1), parameters);
	} else {
		slices.push(vertical ? centerX : centerX - centerY / tanReclaimAngle, size, depth, parameters);
	}
}

template<typename Parameters>
void blendingsimulator::BlendingSimulatorFast<Parameters>::updateHeapMap()
{
	// Either way this is cache inefficient
	for (int z = 0; z < this->heapSizeZ; z++) {
		for (int x = 0; x < this->heapSizeX; x++) {
			const int height = stackedHeights[(x + 1) * rowLength + z + 1];
			this->heapMap[z * this->heapSizeX + x] = height > 0 ? float(height) * realWorldSizeFactor : 0.0f;
		}
	}
}
