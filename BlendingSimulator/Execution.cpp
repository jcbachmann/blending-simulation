#include "Execution.h"

#include <iostream>
#include <sstream>

#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/define_struct.hpp>

namespace qi = boost::spirit::qi;
namespace phx = boost::phoenix;

#include "BlendingSimulatorFast.h"
#include "BlendingSimulatorDetailed.h"
#include "ParticleParameters.h"
#include "BlendingBedVisualizer.h"

BOOST_FUSION_DEFINE_STRUCT(
	(), ParsedLineCounts, (int, pos)(std::vector<int>, counts)
)

BOOST_FUSION_DEFINE_STRUCT(
	(), ParsedLineParameters, (int, pos)(std::vector<double>, parameters)
)

void executeFastSimulationCounted(ExecutionParameters parameters)
{
	// TODO output stuff to other channel
	std::cout << "Initializing simulation" << std::endl;
	BlendingSimulatorFast<CountedParameters> simulator(parameters.length, parameters.depth, parameters.slope, parameters.fourDirectionsOnly);
	std::atomic_bool cancel(false);

	std::cout << "Starting visualization" << std::endl;
	std::thread visualizationThread([&simulator, &cancel]() {
		BlendingBedVisualizer<CountedParameters> visualizer(&simulator);
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

	std::cout << "Starting simulation input" << std::endl;

	std::string line;
	while (std::getline(std::cin, line) && !cancel.load()) {
		auto first = line.begin();
		auto last = line.end();

		ParsedLineCounts parsedLine;
		bool r = qi::phrase_parse(first, last, qi::int_ >> qi::repeat(parameters.parameterCount)[qi::int_],
								  qi::ascii::space, parsedLine);

		if (r && first == last) {
			for (unsigned int i = 0; i < parameters.parameterCount; i++) {
				for (int c = 0; c < parsedLine.counts[i]; c++) {
					simulator.stack(parsedLine.pos, CountedParameters(parameters.parameterCount, i));
				}
				// Artificially slow down stacking in fast simulation to see buildup process
				// std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		} else {
			std::cerr << "could not match line '" << line << "'" << std::endl;
		}
	}

	std::cout << "Simulation input stopped" << std::endl;

	cancel.store(true);
	simulator.finish();

	std::cout << "Simulation finished, waiting for visualization" << std::endl;
	visualizationThread.join();
	std::cout << "Visualization finished" << std::endl;

	int pos;
	CountedParameters p;
	std::vector<int> heights;

	while (simulator.reclaim(pos, p, heights)) {
		if (!parameters.skipPos) {
			std::cout << pos;
		}

		if (!parameters.skipReclaim) {
			for (unsigned int i = 0; i < parameters.parameterCount; i++) {
				std::cout << (i == 0 && parameters.skipPos ? "" : "\t") << p.get(i);
			}
		}

		if (parameters.printHeights) {
			for (auto it = heights.begin(); it != heights.end(); it++) {
				std::cout << (parameters.skipPos && parameters.skipReclaim && it == heights.begin() ? "" : "\t") << *it;
			}
		}

		std::cout << "\n";
	}
}

void executeFastSimulationAveraged(ExecutionParameters parameters)
{
	// TODO add visualization for fast averaged simulation

	BlendingSimulatorFast<AveragedParameters> simulator(parameters.length, parameters.depth, parameters.slope,
														parameters.fourDirectionsOnly);

	std::string line;
	while (std::getline(std::cin, line)) {
		auto first = line.begin();
		auto last = line.end();

		ParsedLineParameters parsedLine;
		bool r = qi::phrase_parse(first, last, qi::int_ >> qi::repeat(parameters.parameterCount)[qi::double_],
								  qi::ascii::space, parsedLine);

		if (r && first == last) {
			simulator.stack(parsedLine.pos, AveragedParameters(std::move(parsedLine.parameters)));
		} else {
			std::cerr << "could not match line '" << line << "'" << std::endl;
		}
	}

	simulator.finish();

	int pos;
	AveragedParameters p;
	std::vector<int> heights;

	while (simulator.reclaim(pos, p, heights)) {
		if (!parameters.skipPos) {
			std::cout << pos;
		}

		if (!parameters.skipReclaim) {
			std::cout << (parameters.skipPos ? "" : "\t") << p.getCount();

			for (unsigned int i = 0; i < parameters.parameterCount; i++) {
				std::cout << "\t" << p.get(i);
			}
		}

		if (parameters.printHeights) {
			for (auto it = heights.begin(); it != heights.end(); it++) {
				std::cout << (parameters.skipPos && parameters.skipReclaim && it == heights.begin() ? "" : "\t") << *it;
			}
		}

		std::cout << "\n";
	}
}

void executeDetailedSimulationCounted(ExecutionParameters parameters)
{
	// TODO output stuff to other channel
	std::cout << "Initializing simulation" << std::endl;
	BlendingSimulatorDetailed<CountedParameters> simulator(parameters.length, parameters.depth);
	std::atomic_bool cancel(false);

	std::cout << "Starting visualization" << std::endl;
	std::thread visualizationThread([&simulator, &cancel]() {
		BlendingBedVisualizer<CountedParameters> visualizer(&simulator);
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

	std::cout << "Starting simulation input" << std::endl;

	std::string line;
	while (std::getline(std::cin, line) && !cancel.load()) {
		auto first = line.begin();
		auto last = line.end();

		ParsedLineCounts parsedLine;
		bool r = qi::phrase_parse(first, last, qi::int_ >> qi::repeat(parameters.parameterCount)[qi::int_],
								  qi::ascii::space, parsedLine);

		if (r && first == last) {
			for (unsigned int i = 0; i < parameters.parameterCount; i++) {
				for (int c = 0; c < parsedLine.counts[i]; c++) {
					simulator.stack(parsedLine.pos, CountedParameters(parameters.parameterCount, i));
				}
			}
		} else {
			std::cerr << "could not match line '" << line << "'" << std::endl;
		}
	}

	std::cout << "Simulation input stopped" << std::endl;

	cancel.store(true);
	simulator.finish();

	std::cout << "Simulation finished, waiting for visualization" << std::endl;
	visualizationThread.join();
	std::cout << "Visualization finished" << std::endl;

	// TODO implement reclaim for detailed simulation
//	int pos;
//	CountedParameters p;
//	std::vector<int> heights;
//
//	while (simulator.reclaim(pos, p, heights)) {
//		if (!parameters.skipPos) {
//			std::cout << pos;
//		}
//
//		if (!parameters.skipReclaim) {
//			for (unsigned int i = 0; i < parameters.parameterCount; i++) {
//				std::cout << (i == 0 && parameters.skipPos ? "" : "\t") << p.get(i);
//			}
//		}
//
//		if (parameters.printHeights) {
//			for (auto it = heights.begin(); it != heights.end(); it++) {
//				std::cout << (parameters.skipPos && parameters.skipReclaim && it == heights.begin() ? "" : "\t") << *it;
//			}
//		}
//
//		std::cout << "\n";
//	}
}

void executeDetailedSimulationAveraged(ExecutionParameters parameters)
{
	// TODO add visualization for detailed averaged simulation

	BlendingSimulatorDetailed<AveragedParameters> simulator(parameters.length, parameters.depth);

	std::string line;
	while (std::getline(std::cin, line)) {
		auto first = line.begin();
		auto last = line.end();

		ParsedLineParameters parsedLine;
		bool r = qi::phrase_parse(first, last, qi::int_ >> qi::repeat(parameters.parameterCount)[qi::double_],
								  qi::ascii::space, parsedLine);

		if (r && first == last) {
			simulator.stack(parsedLine.pos, AveragedParameters(std::move(parsedLine.parameters)));
		} else {
			std::cerr << "could not match line '" << line << "'" << std::endl;
		}
	}

	simulator.finish();

	// TODO implement reclaim for detailed simulation
//	int pos;
//	AveragedParameters p;
//	std::vector<int> heights;
//
//	while (simulator.reclaim(pos, p, heights)) {
//		if (!parameters.skipPos) {
//			std::cout << pos;
//		}
//
//		if (!parameters.skipReclaim) {
//			std::cout << (parameters.skipPos ? "" : "\t") << p.getCount();
//
//			for (unsigned int i = 0; i < parameters.parameterCount; i++) {
//				std::cout << "\t" << p.get(i);
//			}
//		}
//
//		if (parameters.printHeights) {
//			for (auto it = heights.begin(); it != heights.end(); it++) {
//				std::cout << (parameters.skipPos && parameters.skipReclaim && it == heights.begin() ? "" : "\t") << *it;
//			}
//		}
//
//		std::cout << "\n";
//	}
}
