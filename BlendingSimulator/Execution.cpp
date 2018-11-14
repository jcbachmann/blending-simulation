#include "Execution.h"

#include <iostream>
#include <sstream>

#include "BlendingSimulatorFast.h"
#include "BlendingSimulatorDetailed.h"
#include "ParticleParameters.h"

#ifdef VISUALIZER_AVAILABLE
#include "BlendingVisualizer.h"
#endif

void executeSimulation(BlendingSimulator<AveragedParameters>& simulator, ExecutionParameters parameters)
{
	std::cerr << "Initializing simulation" << std::endl;
	std::thread visualizationThread;
	std::atomic_bool cancel(false);

#ifdef VISUALIZER_AVAILABLE
	if (parameters.visualize) {
		std::cerr << "Starting visualization" << std::endl;
		visualizationThread = std::thread([&simulator, &cancel, parameters]() {
			BlendingVisualizer<AveragedParameters> visualizer(&simulator, parameters.verbose, parameters.pretty);
			try {
				visualizer.run();
			} catch (std::exception& e) {
				std::cerr << "Error during execution of visualizer.run(): " << e.what() << std::endl;
			}

			bool wasCancelled = cancel.exchange(true);
			if (!wasCancelled) {
				std::cerr << "Stopping simulation input" << std::endl;
			}
		});
	}
#endif

	std::cerr << "Starting stacking from stdin" << std::endl;

	std::string line;
	int parameterCount = -1;
	while (std::getline(std::cin, line) && !cancel.load()) {
		std::stringstream lineStream(line);

		try {
			double time;
			if (!(lineStream >> time)) {
				throw std::runtime_error("invalid time");
			}

			float xPos;
			if (!(lineStream >> xPos)) {
				throw std::runtime_error("invalid x position");
			}

			float zPos;
			if (!(lineStream >> zPos)) {
				throw std::runtime_error("invalid z position");
			}

			double volume;
			if (!(lineStream >> volume)) {
				throw std::runtime_error("invalid volume");
			}

			std::vector<double> values;

			if (parameterCount >= 0) {
				values.resize(static_cast<unsigned long>(parameterCount));
				for (unsigned int i = 0; i < parameterCount; i++) {
					if (!(lineStream >> values[i])) {
						throw std::runtime_error("invalid value at position " + std::to_string(i));
					}
				}

				std::string trash;
				if (lineStream >> trash) {
					throw std::runtime_error("non-empty line after parsing all parameters");
				}
			} else {
				// Determine parameter count
				double value;
				while (lineStream >> value) {
					// Slow but executed only for first row and parameter count is usually very low
					values.push_back(value);
				}
				parameterCount = static_cast<int>(values.size());
			}

			simulator.stack(xPos, zPos, AveragedParameters(volume, values));
		} catch (std::exception& e) {
			std::cerr << "could not match line '" << line << "': " << e.what() << std::endl;
		}
	}

	std::cerr << "Stacking input stopped" << std::endl;

	cancel.store(true);
	simulator.finishStacking();

	std::cerr << "Stacking finished" << std::endl;

#ifdef VISUALIZER_AVAILABLE
	if (parameters.visualize) {
		std::cerr << "Waiting for visualization" << std::endl;
		visualizationThread.join();
		std::cerr << "Visualization finished" << std::endl;
	}
#endif

	if (!parameters.heightsFile.empty()) {
		std::cerr << "Writing height map into '" << parameters.heightsFile << "'" << std::endl;
		std::ofstream out(parameters.heightsFile);

		if (out) {
			auto heapMapSize = simulator.getHeapMapSize();
			const float* heapMap = simulator.getHeapMap(); // +1 for Y coordinate
			for (int z = 0; z < heapMapSize.second; z++) {
				for (int x = 0; x < heapMapSize.first; x++) {
					if (x > 0) {
						out << "\t";
					}
					out << heapMap[z * heapMapSize.first + x + 1];
				}
				out << "\n";
			}
			out.close();
			std::cerr << "Height map written" << std::endl;
		} else {
			std::cerr << "Could not open output file stream for filename '" << parameters.heightsFile << "'" << std::endl;
		}
	}

	if (!parameters.reclaimFile.empty()) {
		std::cerr << "Reclaiming into '" << parameters.reclaimFile << "'" << std::endl;

		std::streambuf* buf;
		std::ofstream of;
		if (parameters.reclaimFile == "stdout") {
			buf = std::cout.rdbuf();
		} else {
			of.open(parameters.reclaimFile);
			buf = of.rdbuf();
		}
		std::ostream out(buf);

		if (out) {
			out << "position\tvolume";
			for (unsigned int i = 0; i < parameterCount; i++) {
				out << "\tp_" << (i + 1);
			}
			out << "\n";

			float position = 0.0f;
			while (!simulator.reclaimingFinished()) {
				AveragedParameters p = simulator.reclaim(position);

				out << position << "\t" << p.getVolume();
				for (unsigned int i = 0; i < parameterCount; i++) {
					out << "\t" << p.getValue(i);
				}
				out << "\n";

				position += parameters.reclaimIncrement;
			}
			out.flush();
			if (of) {
				of.close();
			}
			std::cerr << "Reclaiming finished" << std::endl;
		} else {
			std::cerr << "Could not open output file stream for filename '" << parameters.reclaimFile << "'" << std::endl;
		}
	}
}

void executeSimulation(ExecutionParameters executionParameters, SimulationParameters simulationParameters)
{
	if (executionParameters.detailed) {
		BlendingSimulatorDetailed<AveragedParameters> simulator(simulationParameters);
		executeSimulation(simulator, executionParameters);
	} else {
		BlendingSimulatorFast<AveragedParameters> simulator(simulationParameters);
		executeSimulation(simulator, executionParameters);
	}
}
