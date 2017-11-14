#include <fstream>

template<typename Parameters>
BlendingSimulator<Parameters>::BlendingSimulator(void)
	: paused(false)
{
}

template<typename Parameters>
void BlendingSimulator<Parameters>::pause()
{
	paused = true;
}

template<typename Parameters>
void BlendingSimulator<Parameters>::resume()
{
	paused = false;
}

template<typename Parameters>
bool BlendingSimulator<Parameters>::isPaused()
{
	return paused.load();
}
