#include "Execution.h"

#include <iostream>
#include <sstream>

#include "BlendingSimulatorFast.h"
#include "BlendingSimulatorDetailed.h"
#include "BlendingVisualizer.h"

void executeSimulation(BlendingSimulator<AveragedParameters>& simulator, ExecutionParameters parameters)
{
	std::cout << "Initializing simulation" << std::endl;
	std::thread visualizationThread;
	std::atomic_bool cancel(false);

	if (parameters.visualize) {
		std::cout << "Starting visualization" << std::endl;
		visualizationThread = std::thread([&simulator, &cancel, parameters]() {
			BlendingVisualizer<AveragedParameters> visualizer(&simulator, parameters.verbose, parameters.pretty);
			try {
				visualizer.run();
			} catch (std::exception& e) {
				std::cerr << "Error during execution of visualizer.run(): " << e.what() << std::endl;
			}

			bool wasCancelled = cancel.exchange(true);
			if (!wasCancelled) {
				std::cout << "Stopping simulation input" << std::endl;
			}
		});
	}

	std::cout << "Starting simulation input" << std::endl;

	std::string line;
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

			std::vector<double> values(parameters.parameterCount);
			for (unsigned int i = 0; i < parameters.parameterCount; i++) {
				if (!(lineStream >> values[i])) {
					throw std::runtime_error("invalid value at position " + std::to_string(i));
				}
			}
			simulator.stack(xPos, zPos, AveragedParameters(volume, std::move(values)));
		} catch (std::exception& e) {
			std::cerr << "could not match line '" << line << "': " << e.what() << std::endl;
		}
	}

	std::cout << "Simulation input stopped" << std::endl;

	cancel.store(true);
	simulator.finishStacking();

	std::cout << "Simulation finished" << std::endl;

	if (parameters.visualize) {
		std::cout << "Waiting for visualization" << std::endl;
		visualizationThread.join();
		std::cout << "Visualization finished" << std::endl;
	}

	if (!parameters.heightsFile.empty()) {
		std::cout << "Writing height map into '" << parameters.heightsFile << "'" << std::endl;
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
			std::cout << "Height map written" << std::endl;
		} else {
			std::cerr << "Could not open output file stream for filename '" << parameters.heightsFile << "'" << std::endl;
		}
	}

	if (!parameters.reclaimFile.empty()) {
		std::cout << "Reclaiming into '" << parameters.reclaimFile << "'" << std::endl;
		std::ofstream out(parameters.reclaimFile);

		if (out) {
			out << "position\tvolume";
			for (unsigned int i = 0; i < parameters.parameterCount; i++) {
				out << "\tp" << (i + 1);
			}
			out << "\n";

			float position = 0.0f;
			while (!simulator.reclaimingFinished()) {
				AveragedParameters p = simulator.reclaim(position);

				out << position << "\t" << p.getVolume();
				for (unsigned int i = 0; i < parameters.parameterCount; i++) {
					out << "\t" << p.getValue(i);
				}
				out << "\n";

				position += parameters.reclaimIncrement;
			}
			out.close();
			std::cout << "Reclaiming finished" << std::endl;
		} else {
			std::cerr << "Could not open output file stream for filename '" << parameters.reclaimFile << "'" << std::endl;
		}
	}
}

void executeSimulation(ExecutionParameters parameters)
{
	if (parameters.detailed) {
		BlendingSimulatorDetailed<AveragedParameters> simulator(parameters.length, parameters.depth, parameters.reclaimAngle, parameters.bulkDensity,
			parameters.particlesPerCubicMeter, parameters.dropHeight, parameters.visualize);
		executeSimulation(simulator, parameters);
	} else {
		BlendingSimulatorFast<AveragedParameters> simulator(parameters.length, parameters.depth, parameters.reclaimAngle, parameters.eightLikelihood,
			parameters.particlesPerCubicMeter, parameters.visualize);
		executeSimulation(simulator, parameters);
	}
}