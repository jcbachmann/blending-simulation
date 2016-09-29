#include <iostream>
#include <string>

#include "Processing.h"

int main(int argc, char** argv) try
{
	if (argc != 2) {
		std::cout << "Invalid argument count, usage: " << argv[0] << " <stacker/reclaimer results files>" << std::endl;
		return 1;
	}

	ProcessingConfig config("processing.config");

	for (int i = 1; i < argc; i++) {
		std::string filename = argv[i];

		if (filename.find("stack.csv") != std::string::npos) {
			processStackerFile(filename, config);
		} else if (filename.find("reclaim.csv") != std::string::npos) {
			processReclaimerFile(filename, config);
		} else {
			throw std::runtime_error("Unknown file type");
		}
	}
} catch (std::exception& e) {
	std::cerr << e.what() << std::endl;
	return 1;
}
