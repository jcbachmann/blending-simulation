#include <iostream>
#include <sstream>

#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/define_struct.hpp>

#include "Simulator.h"
#include "Parameters.h"

namespace po = boost::program_options;
namespace qi = boost::spirit::qi;
namespace phx = boost::phoenix;

BOOST_FUSION_DEFINE_STRUCT(
		(), ParsedLineCounts, (int, pos)(std::vector<int>, counts)
)

BOOST_FUSION_DEFINE_STRUCT(
		(), ParsedLineParameters, (int, pos)(std::vector<double>, parameters)
)

void in_range(unsigned int value, unsigned int min, unsigned int max)
{
	if (value < min || value > max) {
		throw std::runtime_error("value " + std::to_string(value) + " out of range [" + std::to_string(min) + ", " + std::to_string(max) + "]");
	}
}

int main(int argc, char **argv) try
{
	bool printHeights = false;
	bool skipReclaim = false;
	bool skipPos = false;
	bool fourDirectionsOnly = false;
	bool useCounting = false;

	po::options_description desc("Options");
	desc.add_options()
			("help", "produce help message")
			("parameters,n", po::value<unsigned int>()->required()->notifier(boost::bind(&in_range, _1, 1u, 1000000u)), "parameter count")
			("length,l", po::value<unsigned int>()->required()->notifier(boost::bind(&in_range, _1, 1u, 1000000u)), "set stockpile length")
			("depth,d", po::value<unsigned int>()->required()->notifier(boost::bind(&in_range, _1, 1u, 1000000u)), "set stockpile depth")
			("slope,s", po::value<unsigned int>()->default_value(1)->notifier(boost::bind(&in_range, _1, 0u, 1000000u)), "set reclaimer slope")
			("heights,h", po::bool_switch(&printHeights), "output vertical height map")
			("skipreclaim,r", po::bool_switch(&skipReclaim), "skip reclaimer output")
			("skippos,p", po::bool_switch(&skipPos), "skip position output")
			("four,4", po::bool_switch(&fourDirectionsOnly), "axis aligned fall directions only")
			("counting,c", po::bool_switch(&useCounting), "count class occurences instead of averaging parameters (blending model)")
			("config", po::value<std::string>(), "config file");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);

	if (vm.count("config")) {
		po::store(po::parse_config_file<char>(vm["config"].as<std::string>().c_str(), desc), vm);
	}

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 1;
	}

	po::notify(vm);

	unsigned int parameterCount = vm["parameters"].as<unsigned int>();
	unsigned int length = vm["length"].as<unsigned int>();
	unsigned int depth = vm["depth"].as<unsigned int>();
	unsigned int slope = vm["slope"].as<unsigned int>();

	if (useCounting) {
		Simulator<CountedParameters> simulator(length, depth, slope, fourDirectionsOnly);

		std::string line;
		while (std::getline(std::cin, line)) {
			auto first = line.begin();
			auto last = line.end();

			ParsedLineCounts parsedLine;
			bool r = qi::phrase_parse(first, last, qi::int_ >> qi::repeat(parameterCount)[qi::int_], qi::ascii::space,
									  parsedLine);

			if (r && first == last) {
				for (unsigned int i = 0; i < parameterCount; i++) {
					for (int c = 0; c < parsedLine.counts[i]; c++) {
						simulator.stack(parsedLine.pos, CountedParameters(parameterCount, i));
					}
				}
			} else {
				std::cerr << "could not match line '" << line << "'" << std::endl;
			}
		}

		int pos;
		CountedParameters p;
		std::vector<int> heights;

		while (simulator.reclaim(pos, p, heights)) {
			if (!skipPos) {
				std::cout << pos;
			}

			if (!skipReclaim) {
				for (unsigned int i = 0; i < parameterCount; i++) {
					std::cout << (i == 0 && skipPos ? "" : "\t") << p.get(i);
				}
			}

			if (printHeights) {
				for (auto it = heights.begin(); it != heights.end(); it++) {
					std::cout << (skipPos && skipReclaim && it == heights.begin() ? "" : "\t") << *it;
				}
			}

			std::cout << "\n";
		}
	} else {
		Simulator<AveragedParameters> simulator(length, depth, slope, fourDirectionsOnly);

		std::string line;
		while (std::getline(std::cin, line)) {
			auto first = line.begin();
			auto last = line.end();

			ParsedLineParameters parsedLine;
			bool r = qi::phrase_parse(first, last, qi::int_ >> qi::repeat(parameterCount)[qi::double_], qi::ascii::space, parsedLine);

			if (r && first == last) {
				simulator.stack(parsedLine.pos, AveragedParameters(std::move(parsedLine.parameters)));
			} else {
				std::cerr << "could not match line '" << line << "'" << std::endl;
			}
		}

		int pos;
		AveragedParameters p;
		std::vector<int> heights;

		while (simulator.reclaim(pos, p, heights)) {
			if (!skipPos) {
				std::cout << pos;
			}

			if (!skipReclaim) {
				std::cout << (skipPos ? "" : "\t") << p.getCount();

				for (unsigned int i = 0; i < parameterCount; i++) {
					std::cout << "\t" << p.get(i);
				}
			}

			if (printHeights) {
				for (auto it = heights.begin(); it != heights.end(); it++) {
					std::cout << (skipPos && skipReclaim && it == heights.begin() ? "" : "\t") << *it;
				}
			}

			std::cout << "\n";
		}
	}

	std::cout << std::flush;
} catch (std::exception &e) {
	std::cerr << e.what() << std::endl;
	return 1;
}
