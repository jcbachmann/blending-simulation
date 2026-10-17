#ifndef BLENDINGSIMULATOR_DEM_CONTACTMODEL_H
#define BLENDINGSIMULATOR_DEM_CONTACTMODEL_H

#include "BlendingSimulator/dem/Vec3.h"

namespace blendingsimulator::dem
{
// Material of the particles and of the ground. The defaults describe a coarse-grained, free-flowing bulk material such as iron ore or coal
// in parcels of 0.25 to 1 m: friction and restitution in the range of calibrated DEM studies of such materials, rolling resistance for an
// angle of repose of about 45° (see the calibration in the README), and a Young's modulus reduced to a value where particles at the
// bottom of a 10 m pile overlap by about 1 to 2 % of their diameter, a common practice in DEM that keeps the time step affordable.
struct Material
{
	double density = 2600.0; // Density of the particle solid in kg/m³ (not the bulk density of the pile)
	double youngsModulus = 2.0e8; // in Pa
	double poissonRatio = 0.25;
	double restitution = 0.3; // Coefficient of restitution of particle-particle and particle-ground impacts
	double friction = 0.6; // Coefficient of sliding friction between particles
	double rollingFriction = 0.4; // Coefficient of rolling resistance between particles (dimensionless, EPSD model)
	double groundFriction = 0.6; // Sliding friction between particles and the ground
	double groundRollingFriction = 0.4; // Rolling resistance between particles and the ground
	double rollingDamping = 0.3; // Damping ratio of the rolling resistance (EPSD, Ai et al. recommend 0.3)
};

// History of a contact, carried from one time step to the next: the elastic tangential displacement and the elastic rolling torque.
// partner is the index of the other particle, or groundPartner for the ground.
struct ContactHistory
{
	static constexpr int groundPartner = -1;

	int partner = groundPartner;
	Vec3 shear;
	Vec3 rollingTorque;
};

// Geometry and kinematics of one contact, seen from the particle the forces act on
struct ContactState
{
	Vec3 normal; // Unit vector from the particle towards its partner
	double overlap = 0.0; // Sum of the radii minus the distance of the centers in m, positive in contact
	double radius = 0.0; // Radius of the particle in m
	double effectiveRadius = 0.0; // r1 r2 / (r1 + r2), the particle radius for the ground
	double effectiveMass = 0.0; // m1 m2 / (m1 + m2), the particle mass for the ground and for fixed partners
	Vec3 relativeVelocity; // Velocity of the contact point on the particle minus that on the partner
	Vec3 relativeAngularVelocity; // Angular velocity of the particle minus that of the partner
	double rollingInertia = 0.0; // 1 / (1 / (I1 + m1 r1^2) + 1 / (I2 + m2 r2^2)), I1 + m1 r1^2 for the ground and fixed partners
	double friction = 0.0;
	double rollingFriction = 0.0;
};

struct ContactForce
{
	Vec3 force; // On the particle, in N
	Vec3 torque; // On the particle around its center, in N m
	Vec3 tangentialForce; // The tangential part of force
	Vec3 rollingTorque; // The rolling resistance part of torque
	double normalForce = 0.0; // Magnitude of the repulsive normal force in N
};

// Soft-sphere contact model:
//
// * normal: Hertz with the viscous damping of Tsuji et al. (1992), proportional to the square root of the tangent stiffness and the mass
//   as in the Hertz-Mindlin model of EDEM and LIGGGHTS; the restitution does not depend on the impact velocity. The force never attracts
//   (no cohesion). The damping coefficient is solved from the normalized impact equation so that a head-on impact has exactly the
//   material's restitution: the closed form of EDEM and LIGGGHTS, taken from the linear spring, gives 0.25 instead of 0.1 and 0.40 instead
//   of 0.3 once attraction is excluded
// * tangential: Mindlin's no-slip stiffness with damping, an elastic shear displacement carried over the contact's lifetime, and Coulomb's
//   limit, so that piles can rest on slopes (a purely viscous friction cannot hold a static slope)
// * rolling: the elastic-plastic spring-dashpot (EPSD) rolling resistance of Ai et al. (2011, Powder Technology 206), which lets spheres
//   stand for irregular particles: the torque builds up elastically with the relative rotation, is limited to rollingFriction times the
//   effective radius times the normal force, and is damped while below that limit. It acts on the relative rotation about every axis,
//   twisting about the contact normal included, which nothing else would resist
//
// The forces depend on the history, which is updated in place. The computation is antisymmetric: the partner, seeing the negated normal,
// velocities and history, obtains exactly the negated force, so that Newton's third law holds exactly when both sides compute a contact.
class ContactModel
{
	public:
		ContactModel() = default;
		explicit ContactModel(const Material& material);

		ContactForce compute(const ContactState& state, ContactHistory& history, double timeStep) const;

		// Effective Young's and shear modulus of two particles of this material
		double effectiveYoungsModulus() const { return effectiveYoungs; }
		double effectiveShearModulus() const { return effectiveShear; }

	private:
		double effectiveYoungs = 0.0;
		double effectiveShear = 0.0;
		double dampingFactor = 0.0; // Normal and tangential damping per sqrt(tangent stiffness * effective mass)
		double rollingDamping = 0.0;

	public:
		double getDampingFactor() const { return dampingFactor; }

		// Restitution of a head-on impact for a damping factor, from the normalized impact u'' = -u^(3/2) - sqrt(3/2) c u^(1/4) u'
		// with the force limited to repulsion; independent of the impact velocity, masses, radii and moduli
		static double restitutionOf(double dampingFactor);

		// The damping factor for a restitution, bisected with restitutionOf and cached
		static double dampingFactorFor(double restitution);
};
}

#endif
