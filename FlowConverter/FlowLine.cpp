#include "FlowLine.h"

std::istream&operator>>(std::istream& is, FlowLine& fl)
{
	is >> fl.timestamp;
	is >> fl.background;
	is >> fl.red;
	is >> fl.blue;
	is >> fl.yellow;
	is >> fl.undefined;
	is >> fl.pos;

	return is;
}
