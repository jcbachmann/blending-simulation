#include <iostream>

#include <CLI/CLI.hpp>

#include "Execution.h"
#include "BlendingSimulator/SimulationParameters.h"

int main(int argc, char* argv[]) try
{
	ExecutionParameters executionParameters;
	blendingsimulator::SimulationParameters simulationParameters;

	CLI::App app{"Blending Simulator CLI"};
	std::string configFile;

	// Generic Options
	app.add_flag("-v,--verbose", executionParameters.verbose, "Verbose debug output");
	app.set_config("--config")->check(CLI::ExistingFile);

	// Simulation Options
	app.add_flag("--detailed", executionParameters.detailed, "Detailed simulation")
		->group("Simulation Options");
	app.add_flag("--circular", simulationParameters.circular, "Circular simulation")
		->group("Simulation Options");
	app.add_option("-l,--length", simulationParameters.heapWorldSizeX, "Blending bed length", true)
		->group("Simulation Options")
		->check(CLI::Range(0.0f, 1000000.0f));
	app.add_option("-d,--depth", simulationParameters.heapWorldSizeZ, "Blending bed depth", true)
		->group("Simulation Options")
		->check(CLI::Range(0.0f, 1000000.0f));
	app.add_option("--reclaimangle", simulationParameters.reclaimAngle, "Reclaimer angle", true)
		->group("Simulation Options")
		->check(CLI::Range(0.0f, 180.0f));
	app.add_option("--eight", simulationParameters.eightLikelihood, "Likelihood of 8 vs 4 sides being considered", true)
		->group("Simulation Options")
		->check(CLI::Range(0.0f, 1.0f));
	app.add_option("--bulkdensity", simulationParameters.bulkDensityFactor, "Factor for bulk density determination", true)
		->group("Simulation Options")
		->check(CLI::Range(0.001f, 1000.0f));
	app.add_option("--ppm3", simulationParameters.particlesPerCubicMeter, "Amount of particles per cubic meter", true)
		->group("Simulation Options")
		->check(CLI::Range(0.001f, 1000.0f));
	app.add_option("--dropheight", simulationParameters.dropHeight, "Stacker drop height", true)
		->group("Simulation Options")
		->check(CLI::Range(0.001f, 1000.0f));
	app.add_option("--reclaimincrement", executionParameters.reclaimIncrement, "Reclaimer position increment", true)
		->group("Simulation Options")
		->check(CLI::Range(0.001f, 1000.0f));

#ifdef VISUALIZER_AVAILABLE
	// Visualization Options
	app.add_flag("--visualize", executionParameters.visualize, "Show visualization")
		->group("Visualization Options");
	app.add_flag("--pretty", executionParameters.pretty, "Render nicer landscape and details")
		->group("Visualization Options");
#endif

	// Input / Output Options
	app.add_option("--heights", executionParameters.heightsFile, "Height map output file")
		->group("Input / Output Options");
	app.add_option("--reclaim", executionParameters.reclaimFile, "Reclaim output file")
		->group("Input / Output Options");

	try {
		app.parse(argc, argv);
		std::cerr << "----- CONFIG -----\n";
		std::cerr << app.config_to_str(true, true) << "\n";
		std::cerr << "------------------" << std::endl;
	} catch (const CLI::ParseError& e) {
		return app.exit(e);
	}

#ifdef VISUALIZER_AVAILABLE
	simulationParameters.visualize = executionParameters.visualize;
#endif
	executeSimulation(executionParameters, simulationParameters);
} catch (std::exception& e) {
	std::cerr << e.what() << std::endl;
	return 1;
}
