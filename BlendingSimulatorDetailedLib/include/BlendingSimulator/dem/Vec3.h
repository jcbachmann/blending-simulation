#ifndef BLENDINGSIMULATOR_DEM_VEC3_H
#define BLENDINGSIMULATOR_DEM_VEC3_H

#include <cmath>

namespace blendingsimulator::dem
{
// Three-dimensional vector of the discrete element method: x along the bed, y upwards, z across the bed, all in SI units
struct Vec3
{
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;

	constexpr Vec3() = default;

	constexpr Vec3(double x, double y, double z)
		: x(x)
		, y(y)
		, z(z)
	{
	}

	constexpr Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
	constexpr Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
	constexpr Vec3 operator-() const { return {-x, -y, -z}; }
	constexpr Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
	constexpr Vec3 operator/(double s) const { return {x / s, y / s, z / s}; }

	Vec3& operator+=(const Vec3& o)
	{
		x += o.x;
		y += o.y;
		z += o.z;
		return *this;
	}

	Vec3& operator-=(const Vec3& o)
	{
		x -= o.x;
		y -= o.y;
		z -= o.z;
		return *this;
	}

	Vec3& operator*=(double s)
	{
		x *= s;
		y *= s;
		z *= s;
		return *this;
	}
};

constexpr Vec3 operator*(double s, const Vec3& v)
{
	return v * s;
}

constexpr double dot(const Vec3& a, const Vec3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

constexpr Vec3 cross(const Vec3& a, const Vec3& b)
{
	return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

constexpr double squaredNorm(const Vec3& v)
{
	return dot(v, v);
}

inline double norm(const Vec3& v)
{
	return std::sqrt(squaredNorm(v));
}

// The part of v perpendicular to the unit vector n
constexpr Vec3 tangential(const Vec3& v, const Vec3& n)
{
	return v - n * dot(v, n);
}
}

#endif
