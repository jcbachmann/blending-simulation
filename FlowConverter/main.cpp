#include <iostream>

#include <boost/program_options.hpp>
#include <boost/bind.hpp>

#include "Processing.h"

namespace po = boost::program_options;

template<typename T>
void in_range(T value, T min, T max)
{
	if (value < min || value > max) {
		throw std::runtime_error("value " + std::to_string(value) + " out of range [" + std::to_string(min) + ", " + std::to_string(max) + "]");
	}
}

int main(int argc, char** argv) try
{
	po::options_description descGeneric("Generic Options");
	descGeneric.add_options()
		("help", "produce help message")
		("config", po::value<std::string>(), "config file");

	po::options_description descSimulation("Simulation Options");
	descSimulation.add_options()
		("length,l", po::value<unsigned int>()->required()->notifier(boost::bind(&in_range<unsigned int>, _1, 1u, 1000000u)), "blending bed length")
		("depth,d", po::value<unsigned int>()->required()->notifier(boost::bind(&in_range<unsigned int>, _1, 1u, 1000000u)), "blending bed depth");

	po::options_description descModel("Model Options");
	descModel.add_options()
		("reclaim-seconds", po::value<double>()->required()->notifier(boost::bind(&in_range<double>, _1, 1e-6, 1e6)), "reclaim duration used for fine position generation")
		("flow-factor", po::value<double>()->required()->notifier(boost::bind(&in_range<double>, _1, 1e-20, 1e20)), "particle amount multiplier")
		("red-factor", po::value<double>()->required()->notifier(boost::bind(&in_range<double>, _1, 1e-6, 1e6)), "correction factor for red particles")
		("blue-factor", po::value<double>()->required()->notifier(boost::bind(&in_range<double>, _1, 1e-6, 1e6)), "correction factor for blue particles")
		("yellow-factor", po::value<double>()->required()->notifier(boost::bind(&in_range<double>, _1, 1e-6, 1e6)), "correction factor for yellow particles");

	po::options_description descHidden("Hidden Options");
	descHidden.add_options()
		("input-file", po::value<std::vector<std::string>>(), "input file");

	po::positional_options_description positionalOptions;
	positionalOptions.add("input-file", -1);

	po::options_description descAll;
	descAll.add(descGeneric).add(descSimulation).add(descModel).add(descHidden);

	po::options_description showAll;
	showAll.add(descGeneric).add(descSimulation).add(descModel);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(descAll).positional(positionalOptions).run(), vm);

	if (vm.count("config")) {
		po::store(po::parse_config_file<char>(vm["config"].as<std::string>().c_str(), descAll, true), vm);
	}

	if (vm.count("help")) {
		std::cout << "Usage: " << argv[0] << " <stacker/reclaimer results files>" << "\n";
		std::cout << showAll << std::endl;
		return 1;
	}

	po::notify(vm);

	ProcessingConfig config(
		vm["length"].as<unsigned int>(),
		vm["depth"].as<unsigned int>(),
		vm["reclaim-seconds"].as<double>(),
		vm["flow-factor"].as<double>(),
		vm["red-factor"].as<double>(),
		vm["blue-factor"].as<double>(),
		vm["yellow-factor"].as<double>()
	);

	if (!vm.count("input-file")) {
		throw std::runtime_error("no input files specified");
	}

	for (std::string filename : vm["input-file"].as<std::vector<std::string>>()) {
		if (filename.find("stack.csv") != std::string::npos) {
			processStackerFile(filename, config);
		} else if (filename.find("reclaim.csv") != std::string::npos) {
			processReclaimerFile(filename, config);
		} else {
			std::cerr << "skipping unknown file type for filename '" + filename + "'" << std::endl;
		}
	}
} catch (std::exception& e) {
	std::cerr << e.what() << std::endl;
	return 1;
}
