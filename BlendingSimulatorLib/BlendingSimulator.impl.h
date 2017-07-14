#include <fstream>

template<typename Parameters>
BlendingSimulator<Parameters>::BlendingSimulator(void)
	: paused(false)
{
}

template<typename Parameters>
void BlendingSimulator<Parameters>::pause(void)
{
	paused = true;
}

template<typename Parameters>
void BlendingSimulator<Parameters>::resume(void)
{
	paused = false;
}

template<typename Parameters>
bool BlendingSimulator<Parameters>::isPaused(void)
{
	return paused.load();
}

template<typename Parameters>
void BlendingSimulator<Parameters>::loadFromFile(std::string filename)
{
	std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
	unsigned long dataSize = (unsigned long) in.tellg();
	if (dataSize <= 0) {
		throw std::runtime_error("file empty");
	}

	FILE* file = std::fopen(filename.c_str(), "rb");
	if (!file) {
		throw std::runtime_error("cannot read file");
	}

	std::vector<unsigned char> data(dataSize);
	std::fread(data.data(), 1, data.size(), file);
	std::fclose(file);

	setRawData(data);
}

template<typename Parameters>
void BlendingSimulator<Parameters>::saveToFile(std::string filename)
{
	std::vector<unsigned char> data = getRawData();
	if (data.empty()) {
		// No data available
		return;
	}

	FILE* file = std::fopen(filename.c_str(), "wb");
	if (!file) {
		// Could not open file for writing
		return;
	}

	std::fwrite(data.data(), 1, data.size(), file);
	std::fclose(file);
}