#ifndef BLENDINGSIMULATOR_PARTICLELITE_H
#define BLENDINGSIMULATOR_PARTICLELITE_H

#include <tuple>

namespace bs
{
struct Vector3
{
	double x;
	double y;
	double z;

	Vector3()
		: x(0)
		, y(0)
		, z(0)
	{
	}

	Vector3(std::tuple<double, double, double> t)
	{
		x = std::get<0>(t);
		y = std::get<1>(t);
		z = std::get<2>(t);
	}
};

struct Quaternion
{
	double w;
	double x;
	double y;
	double z;

	Quaternion()
		: w(0)
		, x(0)
		, y(0)
		, z(0)
	{
	}

	Quaternion(std::tuple<double, double, double, double> t)
	{
		w = std::get<0>(t);
		x = std::get<1>(t);
		y = std::get<2>(t);
		z = std::get<3>(t);
	}
};
}

template<typename Parameters>
struct ParticleLite
{
	Parameters parameters;
	bool frozen;
	float temperature;

	bs::Vector3 position;
	bs::Vector3 size;
	bs::Quaternion orientation;

	ParticleLite(void)
		: frozen(false)
	{
	}
};

#endif
