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
		("config", po::value<std::string>(), "config file");

	po::options_description descSimulation("Simulation Options");
	descSimulation.add_options()
		("detailed,h", po::bool_switch(&executionParameters.detailed), "detailed simulation")
		("length,l", po::value<unsigned int>()->required()->notifier(boost::bind(&in_range, _1, 1u, 1000000u)), "blending bed length")
		("depth,d", po::value<unsigned int>()->required()->notifier(boost::bind(&in_range, _1, 1u, 1000000u)), "blending bed depth")
		("slope,s", po::value<unsigned int>()->default_value(1)->notifier(boost::bind(&in_range, _1, 0u, 1000000u)), "reclaimer slope")
		("four,4", po::bool_switch(&executionParameters.fourDirectionsOnly), "axis aligned fall directions only");

	po::options_description descInputOutput("Input / Output Options");
	descInputOutput.add_options()
		("parameters,n", po::value<unsigned int>()->required()->notifier(boost::bind(&in_range, _1, 1u, 1000000u)), "particle parameter count")
		("heights,h", po::bool_switch(&executionParameters.printHeights), "output height map")
		("skipreclaim,r", po::bool_switch(&executionParameters.skipReclaim), "skip reclaimer output")
		("skippos,p", po::bool_switch(&executionParameters.skipPos), "skip position output")
		("counting,c", po::bool_switch(&executionParameters.useCounting), "count class occurrences instead of averaging parameters (blending model)");

	po::options_description descAll;
	descAll.add(descGeneric).add(descSimulation).add(descInputOutput);

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

	executionParameters.parameterCount = vm["parameters"].as<unsigned int>();
	executionParameters.length = vm["length"].as<unsigned int>();
	executionParameters.depth = vm["depth"].as<unsigned int>();
	executionParameters.slope = vm["slope"].as<unsigned int>();

	if (executionParameters.detailed) {
		if (executionParameters.useCounting) {
			executeDetailedSimulationCounted(executionParameters);
		} else {
			executeDetailedSimulationAveraged(executionParameters);
		}
	} else {
		if (executionParameters.useCounting) {
			executeFastSimulationCounted(executionParameters);
		} else {
			executeFastSimulationAveraged(executionParameters);
		}
	}

	std::cout << std::flush;
} catch (std::exception& e) {
	std::cerr << e.what() << std::endl;
	return 1;
}
