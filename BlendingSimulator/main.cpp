#include <iostream>

#include <boost/program_options.hpp>
#include <boost/bind.hpp>

#include "Execution.h"

namespace po = boost::program_options;

void in_range(unsigned int value, unsigned int min, unsigned int max)
{
	if (value < min || value > max) {
		throw std::runtime_error("value " + std::to_string(value) + " out of range [" + std::to_string(min) + ", " + std::to_string(max) + "]");
	}
}

int main(int argc, char** argv) try
{
	ExecutionParameters executionParameters;

	po::options_description descGeneric("Generic Options");
	descGeneric.add_options()
		("help", "produce help message")
		("config", po::value<std::string>(), "config file")
		("verbose,v", po::bool_switch(&executionParameters.verbose), "verbose debug output");

	po::options_description descSimulation("Simulation Options");
	descSimulation.add_options()
		("detailed,h", po::bool_switch(&executionParameters.detailed), "detailed simulation")
		("length,l", po::value<float>()->required()->notifier(boost::bind(&in_range, _1, 0.0f, 1000000.0f)), "blending bed length")
		("depth,d", po::value<float>()->required()->notifier(boost::bind(&in_range, _1, 0.0f, 1000000.0f)), "blending bed depth")
		("reclaimangle", po::value<float>()->default_value(45.0f)->notifier(boost::bind(&in_range, _1, 0.0f, 180.0f)), "reclaimer angle")
		("eight", po::value<float>()->default_value(0.87f)->notifier(boost::bind(&in_range, _1, 0.0f, 1.0f)), "likelihood of 8 vs 4 sides being considered")
		("bulkdensity", po::value<float>()->default_value(1.0f)->notifier(boost::bind(&in_range, _1, 0.001f, 1000.0f)), "factor for bulk density determination")
		("ppm3", po::value<float>()->default_value(1.0f)->notifier(boost::bind(&in_range, _1, 0.001f, 1000.0f)), "amount of particles per cubic meter")
		("reclaimincrement", po::value<float>()->default_value(1.0f)->notifier(boost::bind(&in_range, _1, 0.001f, 1000.0f)), "reclaimer position increment");

	po::options_description descVisualization("Visualization Options");
	descVisualization.add_options()
		("visualize", po::bool_switch(&executionParameters.visualize), "show visualization")
		("pretty", po::bool_switch(&executionParameters.pretty), "render nicer landscape and details");

	po::options_description descInputOutput("Input / Output Options");
	descInputOutput.add_options()
		("parameters,n", po::value<unsigned int>()->default_value(0)->notifier(boost::bind(&in_range, _1, 0u, 1000000u)), "particle parameter count")
		("heights,h", po::value<std::string>()->default_value(""), "height map output file")
		("reclaim,r", po::value<std::string>()->default_value(""), "reclaim output file");

	po::options_description descAll;
	descAll.add(descGeneric).add(descSimulation).add(descVisualization).add(descInputOutput);

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

	// Simulation Options
	executionParameters.length = vm["length"].as<float>();
	executionParameters.depth = vm["depth"].as<float>();
	executionParameters.reclaimAngle = vm["reclaimangle"].as<float>();
	executionParameters.eightLikelihood = vm["eight"].as<float>();
	executionParameters.bulkDensity = vm["bulkdensity"].as<float>();
	executionParameters.particlesPerCubicMeter = vm["ppm3"].as<float>();
	executionParameters.reclaimIncrement = vm["reclaimincrement"].as<float>();

	// Input / Output Options
	executionParameters.parameterCount = vm["parameters"].as<unsigned int>();
	executionParameters.heightsFile = vm["heights"].as<std::string>();
	executionParameters.reclaimFile = vm["reclaim"].as<std::string>();

	executeSimulation(executionParameters);
} catch (std::exception& e) {
	std::cerr << e.what() << std::endl;
	return 1;
}
