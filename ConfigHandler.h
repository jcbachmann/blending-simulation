#ifndef CONFIGHANDLER_H
#define CONFIGHANDLER_H

#include <string>
#include <map>
#include <vector>
#include <sstream>

class ConfigHandler
{
	public:
		ConfigHandler(std::string filename);
		~ConfigHandler();

		template <typename T>
		T get(std::string key, T defaultValue)
		{
			std::map<std::string, std::string>::iterator it = vals.find(key);
			if (it != vals.end()) {
				std::istringstream is(it->second);
				T ret;
				if (is >> ret) {
					return ret;
				}
			}

			std::ostringstream os;
			os << defaultValue;
			writeback.emplace_back(key, os.str());
			return defaultValue;
		}

		std::string get(std::string key, std::string defaultValue)
		{
			std::map<std::string, std::string>::iterator it = vals.find(key);
			if (it != vals.end()) {
				return it->second;
			} else {
				writeback.emplace_back(key, defaultValue);
				return defaultValue;
			}
		}

		std::string get(std::string key, const char* defaultValue)
		{
			std::map<std::string, std::string>::iterator it = vals.find(key);
			if (it != vals.end()) {
				return it->second;
			} else {
				writeback.emplace_back(key, defaultValue);
				return defaultValue;
			}
		}

	private:
		std::string filename;
		std::map<std::string, std::string> vals;
		std::vector<std::pair<std::string, std::string>> writeback;
};


#endif
