#ifndef FLOWLINE_H
#define FLOWLINE_H

#include <cstdint>
#include <iostream>

struct FlowLine
{
	uint64_t timestamp;
	double background;
	double red;
	double blue;
	double yellow;
	double undefined;
	float pos; // Relative
};

std::istream& operator>>(std::istream& is, FlowLine& fl);

#endif
