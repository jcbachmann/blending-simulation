#include <limits>
#include <random>
#include <thread>

template<typename Parameters>
BlendingSimulatorFast<Parameters>::BlendingSimulatorFast(SimulationParameters simulationParameters)
	: BlendingSimulator<Parameters>(simulationParameters)
	, reclaimerPos(0.0f)
	, realWorldSizeFactor(1.0f / std::pow(simulationParameters.particlesPerCubicMeter, 1.0f / 3.0f))
{
	if (std::abs(90.0f - simulationParameters.reclaimAngle) < 0.01) {
		tanReclaimAngle = 1e100;
	} else {
		tanReclaimAngle = float(std::tan(simulationParameters.reclaimAngle * std::atan(1.0) * 4.0 / 180.0));
	}

	this->initializeHeapMap(
		(unsigned int) (simulationParameters.heapWorldSizeX / realWorldSizeFactor + 0.5),
		(unsigned int) (simulationParameters.heapWorldSizeZ / realWorldSizeFactor + 0.5)
	);

	// Warning: stackedHeights organized first x than z while heap map is first z than x!
	stackedHeights.resize(this->heapSizeX + 2);
	for (unsigned int x = 0; x < this->heapSizeX + 2; x++) {
		stackedHeights[x].resize(this->heapSizeZ + 2);
	}

	if (simulationParameters.circular) {
		circumference = 2.0 * 3.141592653589793238463 * 0.25 * std::min(simulationParameters.heapWorldSizeX, simulationParameters.heapWorldSizeZ);
		reclaimParameters.resize(static_cast<unsigned int>(circumference / realWorldSizeFactor + 0.5));
	} else {
		reclaimParameters.resize(this->heapSizeX);
	}

	clear();
}

template<typename Parameters>
void BlendingSimulatorFast<Parameters>::clear()
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

	for (Parameters& reclaimParameter : reclaimParameters) {
		reclaimParameter.clear();
	}

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
void BlendingSimulatorFast<Parameters>::finishStacking()
{
	// Nothing to do
}

template<typename Parameters>
bool BlendingSimulatorFast<Parameters>::reclaimingFinished()
{
	return int(reclaimerPos / realWorldSizeFactor + 0.5) >= reclaimParameters.size();
}

template<typename Parameters>
Parameters BlendingSimulatorFast<Parameters>::reclaim(float position)
{
	int startPos = int(reclaimerPos / realWorldSizeFactor + 0.5);
	int endPos = int(position / realWorldSizeFactor + 0.5);
	reclaimerPos = position;

	if (startPos < 0) {
		startPos = 0;
	}

	if (endPos > reclaimParameters.size()) {
		endPos = static_cast<int>(reclaimParameters.size());
	}

	Parameters p;
	for (int i = startPos; i < endPos; i++) {
		p.push(reclaimParameters[i]);
	}
	return p;
}

template<typename Parameters>
void BlendingSimulatorFast<Parameters>::stackSingle(float x, float z, const Parameters& parameters)
{
	while (this->paused.load()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	int xi = std::max(0, std::min(int(x / realWorldSizeFactor + 0.5), int(this->heapSizeX - 1))) + 1;
	int zi = std::max(0, std::min(int(z / realWorldSizeFactor + 0.5), int(this->heapSizeZ - 1))) + 1;

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
		particle->size = bs::Vector3(0.95 * realWorldSizeFactor, 0.95 * realWorldSizeFactor, 0.95 * realWorldSizeFactor);
		particle->orientation = bs::Quaternion(0, 0, 1, 0);

		if (slow) {
			particle->position = bs::Vector3(
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
			{0,  -1},
			{0,  +1},
			{+1, 0},
			{-1, -1},
			{-1, +1},
			{+1, -1},
			{+1, +1}
		};

		static std::random_device rd;
		static std::default_random_engine generator(rd());
		static std::uniform_real_distribution<> coneDistribution(0.0, 1.0);
		const int offsetsCount = coneDistribution(generator) > this->simulationParameters.eightLikelihood ? 4 : 8; // This results in cones instead of pyramids

		static std::uniform_int_distribution<int> randomnessDistribution(0, 8);
		const int r = randomnessDistribution(generator);

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
					particle->position = bs::Vector3(
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

			particle->position = bs::Vector3(
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

	int reclaimIndex = 0;

	// Prepare reclaiming
	if (this->simulationParameters.circular) {
		// Position is along the radius

		double dx = double(xi - 0.5f) * realWorldSizeFactor - 0.5 * this->simulationParameters.heapWorldSizeX + 0.5f;
		double dz = double(zi - 0.5f) * realWorldSizeFactor - 0.5 * this->simulationParameters.heapWorldSizeZ + 0.5f;
		double posOnCircumference = 0.5 * (1.0 - std::atan2(dz, dx) / 3.141592653589793238463) * this->circumference;

		if (tanReclaimAngle > 1e10) {
			// Vertical
		} else if (tanReclaimAngle < 1e-10) {
			// Horizontal
			// Ignore horizontal reclaiming for circular stockpiles - it is useless anyway
		} else {
			posOnCircumference -= float(minHeight) * realWorldSizeFactor / tanReclaimAngle;
		}

		reclaimIndex = (unsigned int) (posOnCircumference / realWorldSizeFactor + 0.5);

		// Acquire positive modulo
		int n = static_cast<int>(reclaimParameters.size());
		reclaimIndex = (reclaimIndex % n + n) % n;
	} else {
		reclaimIndex = xi - 1;

		if (tanReclaimAngle > 1e10) {
			// Vertical
		} else if (tanReclaimAngle < 1e-10) {
			// Horizontal
			if (this->simulationParameters.reclaimAngle < 90.0f) {
				reclaimIndex = 0;
			} else {
				reclaimIndex = static_cast<int>(reclaimParameters.size() - 1);
			}
		} else {
			reclaimIndex -= int(float(minHeight) / tanReclaimAngle + 0.5f);
		}

		if (reclaimIndex < 0) {
			reclaimIndex = 0;
		}

		if (reclaimIndex >= reclaimParameters.size()) {
			reclaimIndex = static_cast<int>(reclaimParameters.size() - 1);
		}
	}

	reclaimParameters[reclaimIndex].push(parameters);
}

template<typename Parameters>
void BlendingSimulatorFast<Parameters>::updateHeapMap()
{
	// Either way this is cache inefficient
	for (int z = 0; z < this->heapSizeZ; z++) {
		for (int x = 0; x < this->heapSizeX; x++) {
			this->heapMap[z * this->heapSizeX + x] = stackedHeights[x + 1][z + 1] > 0 ? float(stackedHeights[x + 1][z + 1]) * realWorldSizeFactor : 0.0f;
		}
	}
}
