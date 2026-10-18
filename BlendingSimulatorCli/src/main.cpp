#include <iostream>

#include <CLI/CLI.hpp>

#include "Execution.h"
#include "BlendingSimulator/SimulationParameters.h"

namespace
{
void addDetailedOptions(CLI::App& app, blendingsimulator::DetailedParameters& d)
{
	const std::string group = "Detailed Simulation Options";
	auto add = [&](const std::string& name, double& value, const std::string& description, double low, double high) {
		app.add_option(name, value, description)->default_val(value)->group(group)->check(CLI::Range(low, high));
	};
	add("--flow", d.flowRate, "Stacker bulk volume flow in m³/s", 1e-6, 1e6);
	add("--dischargeheight", d.dischargeHeight, "Height of the discharge above the pile in m", 0.0, 1000.0);
	add("--beltspeed", d.beltSpeed, "Horizontal speed of the discharged material along +z in m/s", 0.0, 100.0);
	add("--beltwidth", d.beltWidth, "Width of the material stream in m", 0.0, 100.0);
	add("--packing", d.packingFraction, "Solid volume fraction of the settled pile", 0.01, 1.0);
	add("--sizespread", d.sizeSpread, "Relative spread of the particle radii", 0.0, 0.9);
	add("--density", d.density, "Density of the particle solid in kg/m³", 1.0, 1e6);
	add("--youngsmodulus", d.youngsModulus, "Young's modulus of the particles in Pa", 1e3, 1e12);
	add("--poisson", d.poissonRatio, "Poisson's ratio of the particles", 0.0, 0.5);
	add("--restitution", d.restitution, "Coefficient of restitution", 0.0, 1.0);
	add("--friction", d.friction, "Coefficient of sliding friction", 0.0, 10.0);
	add("--rollingfriction", d.rollingFriction, "Coefficient of rolling resistance", 0.0, 10.0);
	add("--rollingdamping", d.rollingDamping, "Damping ratio of the rolling resistance", 0.0, 1.0);
	add("--timestepfactor", d.timeStepFactor, "Time step as a fraction of the Rayleigh time", 1e-3, 1.0);
	add("--restvelocity", d.restVelocity, "Speed below which particles fall asleep in m/s", 0.0, 100.0);
	add("--resttime", d.restTime, "Time below the rest velocity before particles fall asleep in s", 0.0, 1000.0);
	add("--skinfactor", d.skinFactor, "Skin of the pair list as a fraction of the largest radius", 0.01, 10.0);
	app.add_option("--threads", d.threads, "Threads of the detailed simulation, 0 for all cores")
		->default_val(d.threads)
		->group(group)
		->check(CLI::Range(0, 4096));
}
}

int main(const int argc, char* argv[]) try
{
	ExecutionParameters executionParameters;
	blendingsimulator::SimulationParameters simulationParameters;

	CLI::App app{"Blending Simulator CLI"};
	app.set_version_flag("-V,--version", std::string(PROJECT_VERSION));

	// Generic Options
	app.add_flag("-v,--verbose", executionParameters.verbose, "Verbose debug output");
	app.set_config("--config")->check(CLI::ExistingFile);

	// Simulation Options
	app.add_flag("--detailed", executionParameters.detailed, "Detailed simulation")
		->default_val(executionParameters.detailed)
		->group("Simulation Options");
	app.add_flag("--lattice", executionParameters.lattice, "Lattice simulation (hexagonal close-packed)")
		->default_val(executionParameters.lattice)
		->group("Simulation Options");
	app.add_flag("--cone", executionParameters.cone, "Cone simulation (the pile surface at its angle of repose, every input row one portion)")
		->default_val(executionParameters.cone)
		->group("Simulation Options");
	app.add_flag("--formula", executionParameters.formula, "Formula simulation (the cone simulation reduced to the stacker line, no lateral motion)")
		->default_val(executionParameters.formula)
		->group("Simulation Options");
	app.add_flag("--circular", simulationParameters.circular, "Circular simulation")
		->default_val(simulationParameters.circular)
		->group("Simulation Options");
	app.add_option("-l,--length", simulationParameters.heapWorldSizeX, "Blending bed length")
		->default_val(simulationParameters.heapWorldSizeX)
		->group("Simulation Options")
		->check(CLI::Range(0.0f, 1000000.0f));
	app.add_option("-d,--depth", simulationParameters.heapWorldSizeZ, "Blending bed depth")
		->default_val(simulationParameters.heapWorldSizeZ)
		->group("Simulation Options")
		->check(CLI::Range(0.0f, 1000000.0f));
	app.add_option("--reclaimangle", simulationParameters.reclaimAngle, "Reclaimer angle")
		->default_val(simulationParameters.reclaimAngle)
		->group("Simulation Options")
		->check(CLI::Range(0.0f, 180.0f));
	app.add_option("--eight", simulationParameters.eightLikelihood, "Likelihood of 8 vs 4 sides being considered, negative for the value fitted to the particle size")
		->default_val(simulationParameters.eightLikelihood)
		->group("Simulation Options")
		->check(CLI::Range(-1.0f, 1.0f));
	app.add_option("--bulkdensity", simulationParameters.bulkDensityFactor, "Unused since 2026.3 (see --packing), kept for compatibility")
		->default_val(simulationParameters.bulkDensityFactor)
		->group("Simulation Options")
		->check(CLI::Range(0.001f, 1000.0f));
	app.add_option("--ppm3", simulationParameters.particlesPerCubicMeter, "Amount of particles per cubic meter")
		->default_val(simulationParameters.particlesPerCubicMeter)
		->group("Simulation Options")
		->check(CLI::Range(0.001f, 1000.0f));
	app.add_option("--dropheight", simulationParameters.dropHeight, "Unused since 2026.3 (see --dischargeheight), kept for compatibility")
		->default_val(simulationParameters.dropHeight)
		->group("Simulation Options")
		->check(CLI::Range(0.001f, 1000.0f));
	app.add_option("--reclaimincrement", executionParameters.reclaimIncrement, "Reclaimer position increment (default: particle size, 1 / ppm3^(1/3))")
		->group("Simulation Options")
		->check(CLI::Range(0.001f, 1000.0f));
	app.add_option("--seed", simulationParameters.seed, "Random number generator seed for repeatable results (random if not set)")
		->group("Simulation Options");

	// Detailed Simulation Options (discrete element method)
	addDetailedOptions(app, simulationParameters.detailed);

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
