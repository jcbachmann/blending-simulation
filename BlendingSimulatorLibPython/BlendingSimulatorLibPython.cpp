#include <sstream>
#include <iostream>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

#include "BlendingSimulatorFast.h"
#include "BlendingSimulatorDetailed.h"
#include "ParticleParameters.h"
#include "BlendingVisualizer.h"

class VisualizationWrapper
{
	public:
		VisualizationWrapper(BlendingSimulator<AveragedParameters>* simulator, bool verbose, bool pretty)
			: cancel(false)
			, verbose(verbose)
		{
			if (verbose) {
				std::cerr << "Starting visualization" << std::endl;
			}
			visualizationThread = std::thread([this, simulator, verbose, pretty]() {
				BlendingVisualizer<AveragedParameters> visualizer(simulator, verbose, pretty);
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

		~VisualizationWrapper()
		{
			cancel.store(true);

			if (visualizationThread.joinable()) {
				if (verbose) {
					std::cerr << "Waiting for visualization" << std::endl;
				}
				visualizationThread.join();
				if (verbose) {
					std::cerr << "Visualization finished" << std::endl;
				}
			}
		}

	private:
		std::thread visualizationThread;
		std::atomic_bool cancel;
		bool verbose;
};

class BlendingSimulatorLibPython
{
	public:
		BlendingSimulatorLibPython(float heapWorldSizeX, float heapWorldSizeZ, float reclaimAngle, float particlesPerCubicMeter, bool circular,
			float eightLikelihood, bool visualize, float bulkDensityFactor, float dropHeight, bool detailed, float reclaimIncrement, bool pretty)
			: reclaimIncrement(reclaimIncrement)
			, visualizationWrapper(nullptr)
			, verbose(false)
		{
			SimulationParameters simulationParameters{
				heapWorldSizeX,
				heapWorldSizeZ,
				reclaimAngle,
				particlesPerCubicMeter,
				circular,
				eightLikelihood,
				visualize,
				bulkDensityFactor,
				dropHeight
			};

			if (detailed) {
				simulator = new BlendingSimulatorDetailed<AveragedParameters>(simulationParameters);
			} else {
				simulator = new BlendingSimulatorFast<AveragedParameters>(simulationParameters);
			}

			if (visualize) {
				visualizationWrapper = new VisualizationWrapper(simulator, verbose, pretty);
			}
		}

		~BlendingSimulatorLibPython()
		{
			delete simulator;
			delete visualizationWrapper;
		}

		void stack(double timestamp, float x, float z, double volume, const std::vector<double>& parameter)
		{
			simulator->stack(x, z, AveragedParameters(volume, parameter));
		}

		void stackList(const std::vector<std::vector<double>>& data, const std::vector<std::string>& columns, int rows, int cols)
		{
//			int timestampCol = -1;
			int xCol = -1;
			int zCol = -1;
			int volumeCol = -1;
			std::vector<int> parameterColumnIndices;

			for (int i = 0; i < columns.size(); i++) {
				if (columns[i] == "timestamp") {
//					timestampCol = i;
				} else if (columns[i] == "x") {
					xCol = i;
				} else if (columns[i] == "z") {
					zCol = i;
				} else if (columns[i] == "volume") {
					volumeCol = i;
				} else {
					parameterColumnIndices.push_back(i);
					parameterColumns.push_back(columns[i]);
				}
			}

			for (int i = 0; i < rows; i++) {
				std::vector<double> values(parameterColumnIndices.size());
				for (int j = 0; j < parameterColumnIndices.size(); j++) {
					values[j] = data[i][parameterColumnIndices[j]];
				}
				simulator->stack((float)data[i][xCol], (float)data[i][zCol], AveragedParameters(data[i][volumeCol], values));
			}
		}

		py::dict reclaim()
		{
			finishStacking();

			if (verbose) {
				std::cerr << "Reclaiming" << std::endl;
			}

			py::list x;
			py::list volume;
			std::vector<py::list> parameter(parameterColumns.size());

			float position = 0.0f;
			while (!simulator->reclaimingFinished()) {
				AveragedParameters p = simulator->reclaim(position);
				x.append(position);
				volume.append(p.getVolume());
				const auto& values = p.getValues();
				for (int i = 0; i < parameterColumns.size(); i++) {
					parameter[i].append(i < values.size() ? values[i] : 0);
				}
				position += reclaimIncrement;
			}

			py::dict ret("x"_a = x, "volume"_a = volume);
			for (int i = 0; i < parameterColumns.size(); i++) {
				ret[parameterColumns[i].c_str()] = parameter[i];
			}
			return ret;
		}

		std::vector<std::vector<float>> getHeights()
		{
			finishStacking();

			if (verbose) {
				std::cerr << "Acquiring heights" << std::endl;
			}

			auto heapMapSize = simulator->getHeapMapSize();
			std::vector<std::vector<float>> heights;
			heights.reserve(heapMapSize.second);
			const float* heapMap = simulator->getHeapMap(); // +1 for Y coordinate
			for (int z = 0; z < heapMapSize.second; z++) {
				heights.emplace_back(heapMap + z * heapMapSize.first + 1, heapMap + z * heapMapSize.first + 1 + heapMapSize.first);
			}

			return heights;
		}

	private:
		BlendingSimulator<AveragedParameters>* simulator;
		float reclaimIncrement;
		std::vector<std::string> parameterColumns;
		VisualizationWrapper* visualizationWrapper;
		bool verbose;

		void finishStacking()
		{
			simulator->finishStacking();

			if (verbose) {
				std::cerr << "Stacking finished" << std::endl;
			}
		}
};

PYBIND11_MODULE(blending_simulator_lib, m)
{
	m.doc() = "Blending Simulator Lib for Python";

	py::class_<BlendingSimulatorLibPython>(m, "BlendingSimulatorLib")
		.def(py::init<float, float, float, float, bool, float, bool, float, float, bool, float, bool>())
		.def("stack", &BlendingSimulatorLibPython::stack)
		.def("stack_list", &BlendingSimulatorLibPython::stackList)
		.def("reclaim", &BlendingSimulatorLibPython::reclaim)
		.def("get_heights", &BlendingSimulatorLibPython::getHeights);
}
