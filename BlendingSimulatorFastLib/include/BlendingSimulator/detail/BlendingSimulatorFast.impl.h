#include <cmath>
#include <limits>
#include <random>
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
	stackedHeights.resize(this->heapSizeX + 2);
	for (unsigned int x = 0; x < this->heapSizeX + 2; x++) {
		stackedHeights[x].resize(this->heapSizeZ + 2);
	}

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
	std::fill(stackedHeights[0].begin(), stackedHeights[0].end(), std::numeric_limits<int>::max());
	for (unsigned int x = 1; x < this->heapSizeX + 1; x++) {
		for (unsigned int z = 1; z < this->heapSizeZ + 1; z++) {
			stackedHeights[x][z] = 0;
		}
		stackedHeights[x][0] = std::numeric_limits<int>::max();
		stackedHeights[x][this->heapSizeZ + 1] = std::numeric_limits<int>::max();
	}
	std::fill(stackedHeights[this->heapSizeX + 1].begin(), stackedHeights[this->heapSizeX + 1].end(), std::numeric_limits<int>::max());

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
	// Nothing to do
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
void blendingsimulator::BlendingSimulatorFast<Parameters>::stackSingle(float x, float z, const Parameters& parameters)
{
	while (this->paused.load()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	// Cell i (stackedHeights index i + 1 because of the border) covers the positions from i to i + 1 particle sizes
	int xi = std::max(0, std::min(static_cast<int>(std::floor(x / realWorldSizeFactor)), int(this->heapSizeX - 1))) + 1;
	int zi = std::max(0, std::min(static_cast<int>(std::floor(z / realWorldSizeFactor)), int(this->heapSizeZ - 1))) + 1;

	int minHeightX;
	int minHeightZ;
	int minHeight = stackedHeights[xi][zi];

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

	// Simulate particle falling
	do {
		minHeightX = -1;
		minHeightZ = -1;

		struct Offset
		{
			int i;
			int j;
		};
		constexpr const static Offset offsets[] = {
			{-1, 0},
			{0, -1},
			{0, +1},
			{+1, 0},
			{-1, -1},
			{-1, +1},
			{+1, -1},
			{+1, +1}
		};

		const int offsetsCount = coneDistribution(this->randomEngine) > this->simulationParameters.eightLikelihood ? 4 : 8; // This results in cones instead of pyramids

		const int r = directionDistribution(this->randomEngine);

		for (int o = 0; o < offsetsCount; o++) {
			const Offset& offset = offsets[(o + r) % offsetsCount];
			const int tx = xi + offset.i;
			const int tz = zi + offset.j;
			const int lh = stackedHeights[tx][tz];

			if (lh < minHeight) {
				minHeightX = tx;
				minHeightZ = tz;
				minHeight = lh;
			}
		}

		if (minHeightX >= 0) {
			xi = minHeightX;
			zi = minHeightZ;

			if (this->simulationParameters.visualize && slow) {
				{
					std::lock_guard<std::mutex> lock(this->outputParticlesMutex);
					particle->position = Vector3(
						(float(xi) - 0.5f) * realWorldSizeFactor,
						(float(minHeight) + 0.5f) * realWorldSizeFactor,
						(float(zi) - 0.5f) * realWorldSizeFactor
					);
				}

				std::this_thread::sleep_for(simulationSleep);
			}
		}
	} while (minHeightX >= 0);

	// Update height
	stackedHeights[xi][zi] = minHeight + 1;

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
			this->heapMap[z * this->heapSizeX + x] = stackedHeights[x + 1][z + 1] > 0 ? float(stackedHeights[x + 1][z + 1]) * realWorldSizeFactor : 0.0f;
		}
	}
}
