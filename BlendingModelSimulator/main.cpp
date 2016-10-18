#include <iostream>
#include <sstream>
#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <Simulator.h>

void in_range(unsigned int value, unsigned int min, unsigned int max)
{
	if (value < min || value > max) {
		throw std::runtime_error("value " + std::to_string(value) + " out of range [" + std::to_string(min) + ", " + std::to_string(max) + "]");
	}
}

struct Parameters {
	unsigned int red;
	unsigned int blue;
	unsigned int yellow;

	Parameters(void)
			: red(0)
			, blue(0)
			, yellow(0)
	{
	}

	void add(Parameters p)
	{
		red += p.red;
		blue += p.blue;
		yellow += p.yellow;
	}

	void clear(void)
	{
		red = 0;
		blue = 0;
		yellow = 0;
	}
};

int main(int argc, char **argv) try
{
	bool printHeights = false;
	bool skipReclaim = false;
	bool skipPos = false;
	bool fourDirectionsOnly = false;

	boost::program_options::options_description desc("Options");
	desc.add_options()
			("help", "produce help message")
			("length,l", boost::program_options::value<unsigned int>()->required()->notifier(boost::bind(&in_range, _1, 1u, 1000000u)), "set stockpile length")
			("depth,d", boost::program_options::value<unsigned int>()->required()->notifier(boost::bind(&in_range, _1, 1u, 1000000u)), "set stockpile depth")
			("slope,s", boost::program_options::value<unsigned int>()->default_value(1)->notifier(boost::bind(&in_range, _1, 0u, 1000000u)), "set reclaimer slope")
			("heights,h", boost::program_options::bool_switch(&printHeights), "output vertical height map")
			("skipreclaim,r", boost::program_options::bool_switch(&skipReclaim), "skip reclaimer output")
			("skippos,p", boost::program_options::bool_switch(&skipPos), "skip position output")
			("four,4", boost::program_options::bool_switch(&fourDirectionsOnly), "axis aligned fall directions only");

	boost::program_options::variables_map vm;
	boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 1;
	}

	boost::program_options::notify(vm);

	unsigned int length = vm["length"].as<unsigned int>();
	unsigned int depth = vm["depth"].as<unsigned int>();
	unsigned int slope = vm["slope"].as<unsigned int>();

	Simulator<Parameters> simulator(length, depth, slope, fourDirectionsOnly);

	int pos;
	int red;
	int blue;
	int yellow;

	std::string line;
	while (std::getline(std::cin, line)) {
		auto first = line.begin();
		auto last = line.end();
		bool r = boost::spirit::qi::phrase_parse(first, last,
				boost::spirit::qi::int_[boost::phoenix::ref(pos) = boost::spirit::qi::_1]
						>> boost::spirit::qi::int_[boost::phoenix::ref(red) = boost::spirit::qi::_1]
						>> boost::spirit::qi::int_[boost::phoenix::ref(blue) = boost::spirit::qi::_1]
						>> boost::spirit::qi::int_[boost::phoenix::ref(yellow) = boost::spirit::qi::_1],
				boost::spirit::ascii::space);

		if (r && first == last) {
			for (unsigned int i = 0; i < red; i++) {
				Parameters p;
				p.red = 1;
				simulator.stack(pos, p);
			}

			for (unsigned int i = 0; i < blue; i++) {
				Parameters p;
				p.blue = 1;
				simulator.stack(pos, p);
			}

			for (unsigned int i = 0; i < yellow; i++) {
				Parameters p;
				p.yellow = 1;
				simulator.stack(pos, p);
			}
		} else {
			std::cerr << "could not match line '" << line << "'" << std::endl;
		}
	}

	std::vector<int> heights;
	int i = 0;
	Parameters p;

	while (simulator.reclaim(pos, p, heights)) {
		if (!skipPos) {
			std::cout << pos;
		}

		if (!skipReclaim) {
			std::cout << (skipPos ? "" : "\t") << p.red << "\t" << p.blue << "\t" << p.yellow;
		}

		if (printHeights) {
			for (auto it = heights.begin(); it != heights.end(); it++) {
				std::cout << (skipPos && skipReclaim && it == heights.begin() ? "" : "\t") << *it;
			}
		}

		std::cout << "\n";
	}

	std::cout << std::flush;
} catch (std::exception &e) {
	std::cerr << e.what() << std::endl;
	return 1;
}
