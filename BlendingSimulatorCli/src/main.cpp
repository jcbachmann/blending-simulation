#include <iostream>

#include <boost/program_options.hpp>
#include <boost/bind.hpp>

#include "Execution.h"

namespace po = boost::program_options;

void in_range(float value, float min, float max)
{
	if (value < min || value > max) {
		throw std::runtime_error("value " + std::to_string(value) + " out of range [" + std::to_string(min) + ", " + std::to_string(max) + "]");
	}
}

int main(int argc, char* argv[]) try
{
	ExecutionParameters executionParameters;
	SimulationParameters simulationParameters;

	po::options_description descGeneric("Generic Options");
	descGeneric.add_options()
		("help",
			"produce help message")
		("config",
			po::value<std::string>(),
			"config file")
		("verbose,v",
			po::bool_switch(&executionParameters.verbose),
			"verbose debug output"
		);

	po::options_description descSimulation("Simulation Options");
	descSimulation.add_options()
		("detailed",
			po::bool_switch(&executionParameters.detailed),
			"detailed simulation")
		("circular",
			po::bool_switch(&simulationParameters.circular),
			"detailed simulation")
		("length,l",
			po::value<float>(&simulationParameters.heapWorldSizeX)
				->required()
				->notifier(boost::bind(&in_range, _1, 0.0f, 1000000.0f)),
			"blending bed length")
		("depth,d",
			po::value<float>(&simulationParameters.heapWorldSizeZ)
				->required()
				->notifier(boost::bind(&in_range, _1, 0.0f, 1000000.0f)),
			"blending bed depth")
		("reclaimangle",
			po::value<float>(&simulationParameters.reclaimAngle)
				->default_value(45.0f)
				->notifier(boost::bind(&in_range, _1, 0.0f, 180.0f)),
			"reclaimer angle")
		("eight",
			po::value<float>(&simulationParameters.eightLikelihood)
				->default_value(0.87f)
				->notifier(boost::bind(&in_range, _1, 0.0f, 1.0f)),
			"likelihood of 8 vs 4 sides being considered")
		("bulkdensity",
			po::value<float>(&simulationParameters.bulkDensityFactor)
				->default_value(1.0f)
				->notifier(boost::bind(&in_range, _1, 0.001f, 1000.0f)),
			"factor for bulk density determination")
		("ppm3",
			po::value<float>(&simulationParameters.particlesPerCubicMeter)
				->default_value(1.0f)
				->notifier(boost::bind(&in_range, _1, 0.001f, 1000.0f)),
			"amount of particles per cubic meter")
		("dropheight,h",
			po::value<float>(&simulationParameters.dropHeight)
				->required()
				->notifier(boost::bind(&in_range, _1, 0.001f, 1000.0f)),
			"stacker drop height")
		("reclaimincrement",
			po::value<float>(&executionParameters.reclaimIncrement)
				->default_value(1.0f)
				->notifier(boost::bind(&in_range, _1, 0.001f, 1000.0f)),
			"reclaimer position increment");

#ifdef VISUALIZER_AVAILABLE
	po::options_description descVisualization("Visualization Options");
	descVisualization.add_options()
		("visualize",
			po::bool_switch(&executionParameters.visualize),
			"show visualization")
		("pretty",
			po::bool_switch(&executionParameters.pretty),
			"render nicer landscape and details");
#endif

	po::options_description descInputOutput("Input / Output Options");
	descInputOutput.add_options()
		("heights",
			po::value<std::string>(&executionParameters.heightsFile)
				->default_value(""),
			"height map output file")
		("reclaim",
			po::value<std::string>(&executionParameters.reclaimFile)
				->default_value(""),
			"reclaim output file");

	po::options_description descAll;
	descAll.add(descGeneric).add(descSimulation)
#ifdef VISUALIZER_AVAILABLE
		.add(descVisualization)
#endif
		.add(descInputOutput);

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, descAll), vm);

	if (vm.count("config")) {
		po::store(po::parse_config_file<char>(vm["config"].as<std::string>().c_str(), descAll, true), vm);
	}

	if (vm.count("help")) {
		std::cout << descAll << std::endl;
		return 1;
	}

	po::notify(vm);

#ifdef VISUALIZER_AVAILABLE
	simulationParameters.visualize = executionParameters.visualize;
#else
	simulationParameters.visualize = false;
#endif
	executeSimulation(executionParameters, simulationParameters);
} catch (std::exception& e) {
	std::cerr << e.what() << std::endl;
	return 1;
}
