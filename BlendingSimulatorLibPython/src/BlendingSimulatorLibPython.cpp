#include <sstream>
#include <iostream>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

#include "BlendingSimulator/BlendingSimulatorFast.h"
#include "BlendingSimulator/BlendingSimulatorDetailed.h"
#include "BlendingSimulator/ParticleParameters.h"

class BlendingSimulatorLibPython
{
	public:
		BlendingSimulatorLibPython(float heapWorldSizeX, float heapWorldSizeZ, float reclaimAngle, float particlesPerCubicMeter, bool circular,
			float eightLikelihood, float bulkDensityFactor, float dropHeight, bool detailed, float reclaimIncrement)
			: reclaimIncrement(reclaimIncrement)
			, verbose(false)
		{
			SimulationParameters simulationParameters{
				heapWorldSizeX,
				heapWorldSizeZ,
				reclaimAngle,
				particlesPerCubicMeter,
				circular,
				eightLikelihood,
				false, // visualize - not available in python library
				bulkDensityFactor,
				dropHeight
			};

			if (detailed) {
				simulator = new BlendingSimulatorDetailed<AveragedParameters>(simulationParameters);
			} else {
				simulator = new BlendingSimulatorFast<AveragedParameters>(simulationParameters);
			}
		}

		~BlendingSimulatorLibPython()
		{
			delete simulator;
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
		.def(py::init<float, float, float, float, bool, float, float, float, bool, float>())
		.def("stack", &BlendingSimulatorLibPython::stack)
		.def("stack_list", &BlendingSimulatorLibPython::stackList)
		.def("reclaim", &BlendingSimulatorLibPython::reclaim)
		.def("get_heights", &BlendingSimulatorLibPython::getHeights);
}
