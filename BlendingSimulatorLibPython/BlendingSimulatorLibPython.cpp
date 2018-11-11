#include <sstream>
#include <iostream>

#include <boost/python.hpp>

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
			SimulationParameters simulationParameters
			{
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

		void stack(double timestamp, float x, float z, double volume, const boost::python::list& parameter)
		{
			auto len = boost::python::len(parameter);
			std::vector<double> values(static_cast<unsigned long>(len));
			for (int i = 0; i < len; i++) {
				values[i] = boost::python::extract<double>(parameter[i]);
			}
			simulator->stack(x, z, AveragedParameters(volume, std::move(values)));
		}

		void stackList(const boost::python::list& data, const boost::python::list& columns, int rows, int cols)
		{
//			int timestampCol = -1;
			int xCol = -1;
			int zCol = -1;
			int volumeCol = -1;
			std::vector<int> parameterColumnIndices;

			auto len = boost::python::len(columns);
			for (int i = 0; i < len; i++) {
				std::string t = boost::python::extract<std::string>(columns[i]);
				if (t == "timestamp") {
//					timestampCol = i;
				} else if (t == "x") {
					xCol = i;
				} else if (t == "z") {
					zCol = i;
				} else if (t == "volume") {
					volumeCol = i;
				} else {
					parameterColumnIndices.push_back(i);
					parameterColumns.push_back(t);
				}
			}

			for (int i = 0; i < rows; i++) {
				const boost::python::list& row = boost::python::extract<boost::python::list>(data[i]);
				std::vector<double> values(parameterColumnIndices.size());

				for (int j = 0; j < parameterColumnIndices.size(); j++) {
					values[j] = boost::python::extract<double>(row[parameterColumnIndices[j]]);
				}
				simulator->stack(
					boost::python::extract<float>(row[xCol]),
					boost::python::extract<float>(row[zCol]),
					AveragedParameters(boost::python::extract<double>(row[volumeCol]), std::move(values))
				);
			}
		}

		boost::python::dict reclaim()
		{
			finishStacking();

			if (verbose) {
				std::cerr << "Reclaiming" << std::endl;
			}

			boost::python::list x;
			boost::python::list volume;
			std::vector<boost::python::list> parameter(parameterColumns.size());

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

			boost::python::dict ret;
			ret["x"] = x;
			ret["volume"] = volume;
			for (int i = 0; i < parameterColumns.size(); i++) {
				ret[parameterColumns[i]] = parameter[i];
			}
			return ret;
		}

		boost::python::list getHeights()
		{
			finishStacking();

			if (verbose) {
				std::cerr << "Acquiring heights" << std::endl;
			}
			boost::python::list heights;

			auto heapMapSize = simulator->getHeapMapSize();
			const float* heapMap = simulator->getHeapMap(); // +1 for Y coordinate
			for (int z = 0; z < heapMapSize.second; z++) {
				boost::python::list l;
				for (int x = 0; x < heapMapSize.first; x++) {
					l.append(heapMap[z * heapMapSize.first + x + 1]);
				}
				heights.append(l);
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

BOOST_PYTHON_MODULE (blending_simulator_lib)
{
	boost::python::class_<BlendingSimulatorLibPython>("BlendingSimulatorLib",
		boost::python::init<float, float, float, float, bool, float, bool, float, float, bool, float, bool>())
		.def("stack", &BlendingSimulatorLibPython::stack)
		.def("stack_list", &BlendingSimulatorLibPython::stackList)
		.def("reclaim", &BlendingSimulatorLibPython::reclaim)
		.def("get_heights", &BlendingSimulatorLibPython::getHeights);
}
