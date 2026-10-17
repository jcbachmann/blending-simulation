#include "BlendingSimulator/dem/ContactModel.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <mutex>

namespace blendingsimulator::dem
{
namespace
{
// Turns a vector stored in the tangent plane of an earlier step into the current tangent plane, keeping its length: contacts rotate
// slowly, and dropping the normal component alone would shrink the elastic displacement of rolling contacts over time
Vec3 rotateIntoPlane(const Vec3& v, const Vec3& normal)
{
	const double before = squaredNorm(v);
	if (before == 0.0) {
		return v;
	}
	const Vec3 projected = tangential(v, normal);
	const double after = squaredNorm(projected);
	if (after == 0.0) {
		return {};
	}
	return projected * std::sqrt(before / after);
}
}

ContactModel::ContactModel(const Material& material)
{
	const double nu = material.poissonRatio;
	const double shearModulus = material.youngsModulus / (2.0 * (1.0 + nu));
	// Two bodies of the same material: 1/E* = 2 (1 - nu^2) / E and 1/G* = 2 (2 - nu) / G
	effectiveYoungs = material.youngsModulus / (2.0 * (1.0 - nu * nu));
	effectiveShear = shearModulus / (2.0 * (2.0 - nu));
	dampingFactor = dampingFactorFor(std::clamp(material.restitution, 0.0, 1.0));
	rollingDamping = material.rollingDamping;
}

double ContactModel::dampingFactorFor(double restitution)
{
	// The restitution falls monotonically with the damping: bisect for the wanted value, once per value in a process
	static std::mutex mutex;
	static std::map<double, double> cache;
	std::lock_guard<std::mutex> lock(mutex);
	if (auto it = cache.find(restitution); it != cache.end()) {
		return it->second;
	}
	double low = 0.0;
	double high = 1.0;
	while (restitutionOf(high) > restitution && high < 1e3) {
		high *= 2.0;
	}
	for (int i = 0; i < 50 && restitution < 1.0; i++) {
		const double mid = 0.5 * (low + high);
		(restitutionOf(mid) > restitution ? low : high) = mid;
	}
	const double factor = restitution < 1.0 ? 0.5 * (low + high) : 0.0;
	cache[restitution] = factor;
	return factor;
}

double ContactModel::restitutionOf(double c)
{
	// Normalized with the impact speed v and the length (m v^2 / K)^(2/5) of the Hertz force K u^(3/2); integrated with classical
	// Runge-Kutta until the particles separate
	const double damping = std::sqrt(1.5) * c;
	auto acceleration = [damping](double u, double v) {
		if (u <= 0.0) {
			return 0.0;
		}
		return -std::max(0.0, std::pow(u, 1.5) + damping * std::pow(u, 0.25) * v);
	};
	double u = 0.0;
	double v = 1.0;
	const double h = 1e-3;
	for (int i = 0; i < 1000000; i++) {
		const double k1u = v;
		const double k1v = acceleration(u, v);
		const double k2u = v + 0.5 * h * k1v;
		const double k2v = acceleration(u + 0.5 * h * k1u, v + 0.5 * h * k1v);
		const double k3u = v + 0.5 * h * k2v;
		const double k3v = acceleration(u + 0.5 * h * k2u, v + 0.5 * h * k2v);
		const double k4u = v + h * k3v;
		const double k4v = acceleration(u + h * k3u, v + h * k3v);
		const double nextU = u + h / 6.0 * (k1u + 2.0 * k2u + 2.0 * k3u + k4u);
		const double nextV = v + h / 6.0 * (k1v + 2.0 * k2v + 2.0 * k3v + k4v);
		if (nextU <= 0.0 && i > 0) {
			return std::max(0.0, -nextV);
		}
		u = nextU;
		v = nextV;
	}
	return 0.0;
}

ContactForce ContactModel::compute(const ContactState& state, ContactHistory& history, double timeStep) const
{
	ContactForce result;
	const Vec3& n = state.normal;
	const double sqrtRadiusOverlap = std::sqrt(state.effectiveRadius * state.overlap);
	// The damping coefficients go with sqrt(stiffness * mass), and the stiffnesses with sqrt(R* overlap): one fourth root for both
	const double fourthRootRadiusOverlap = std::sqrt(sqrtRadiusOverlap);
	const double sqrtMass = std::sqrt(state.effectiveMass);

	// Normal: Hertz spring and damping; vn > 0 when the particles approach each other
	const double normalStiffness = 4.0 / 3.0 * effectiveYoungs * sqrtRadiusOverlap; // Secant stiffness, F = kn * overlap
	const double normalDamping = dampingFactor * std::sqrt(2.0 * effectiveYoungs) * fourthRootRadiusOverlap * sqrtMass; // tangent S_n = 2 E* sqrt(R* d)
	const double vn = dot(state.relativeVelocity, n);
	const double fn = std::max(0.0, normalStiffness * state.overlap + normalDamping * vn);
	result.normalForce = fn;
	result.force = n * -fn;

	// Tangential: elastic shear displacement with damping, limited by Coulomb friction
	const double tangentialStiffness = 8.0 * effectiveShear * sqrtRadiusOverlap;
	const double tangentialDamping = dampingFactor * std::sqrt(8.0 * effectiveShear) * fourthRootRadiusOverlap * sqrtMass;
	const Vec3 vt = tangential(state.relativeVelocity, n);
	history.shear = rotateIntoPlane(history.shear, n) + vt * timeStep;
	Vec3 ft = history.shear * -tangentialStiffness - vt * tangentialDamping;
	const double ftLimit = state.friction * fn;
	const double ft2 = squaredNorm(ft);
	if (ft2 > ftLimit * ftLimit) {
		// Sliding: the force is at the Coulomb limit and the spring holds just that force
		ft = ft * (ftLimit / std::sqrt(ft2));
		history.shear = tangentialStiffness > 0.0 ? ft * (-1.0 / tangentialStiffness) : Vec3();
	}
	result.force += ft;
	result.tangentialForce = ft;
	result.torque = cross(n * state.radius, ft);

	// Rolling resistance (EPSD): an elastic torque on the relative rotation, limited to mu_r R* F_n, damped below the limit
	if (state.rollingFriction > 0.0) {
		const double rollingStiffness = 2.25 * normalStiffness * state.rollingFriction * state.rollingFriction * state.effectiveRadius *
			state.effectiveRadius;
		// Rolling and twisting together: the relative rotation about any axis, so that a particle also stops spinning about the contact
		// normal (nothing else resists that spin); the tangential part of the stored torque turns with the contact like the shear
		const Vec3 rolling = state.relativeAngularVelocity;
		const Vec3 twist = n * dot(history.rollingTorque, n);
		history.rollingTorque = rotateIntoPlane(tangential(history.rollingTorque, n), n) + twist - rolling * (rollingStiffness * timeStep);
		const double limit = state.rollingFriction * state.effectiveRadius * fn;
		const double torque2 = squaredNorm(history.rollingTorque);
		if (torque2 >= limit * limit) {
			// Fully mobilized: the torque stays at the limit, without damping
			history.rollingTorque = torque2 > 0.0 ? history.rollingTorque * (limit / std::sqrt(torque2)) : Vec3();
			result.rollingTorque = history.rollingTorque;
		} else {
			const double criticalDamping = 2.0 * std::sqrt(state.rollingInertia * rollingStiffness);
			result.rollingTorque = history.rollingTorque - rolling * (rollingDamping * criticalDamping);
		}
		result.torque += result.rollingTorque;
	}
	return result;
}
}
