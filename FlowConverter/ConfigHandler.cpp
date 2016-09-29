#include "ConfigHandler.h"

#include <iostream>
#include <fstream>

std::string trim(const std::string& str, const std::string& whitespace = " \t")
{
	const auto strBegin = str.find_first_not_of(whitespace);
	if (strBegin == std::string::npos) {
		return ""; // no content
	}

	const auto strEnd = str.find_last_not_of(whitespace);
	const auto strRange = strEnd - strBegin + 1;

	return str.substr(strBegin, strRange);
}

ConfigHandler::ConfigHandler(std::string filename)
	: filename(filename)
{
	std::ifstream ifs(filename);
	std::string line;

	while (std::getline(ifs, line)) {
		line = line.substr(0, line.find_first_of('#'));
		size_t eqpos = line.find_first_of('=');

		if (eqpos != std::string::npos) {
			std::string key = trim(line.substr(0, eqpos));
			std::string value = trim(line.substr(eqpos + 1));
			vals[key] = value;
		}
	}
}

ConfigHandler::~ConfigHandler()
{
	if (!writeback.empty()) {
		std::ofstream ofs(filename, std::ofstream::out | std::ofstream::app);
		ofs << '\n';

		for (const auto& a : writeback) {
			ofs << a.first << " = " << a.second << '\n';
		}
	}
}
