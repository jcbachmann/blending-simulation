#ifndef BLENDINGSIMULATOR_DEM_WORLD_H
#define BLENDINGSIMULATOR_DEM_WORLD_H

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "BlendingSimulator/dem/ContactModel.h"
#include "BlendingSimulator/dem/Vec3.h"

namespace blendingsimulator::dem
{
struct WorldSettings
{
	double gravity = 9.80665; // in m/s², along -y
	// Time step as a fraction of the Rayleigh time of the smallest particle: 0.25 is unstable under the load of a stockpile (the Hertz
	// contacts stiffen with the overlap, and a sphere with a dozen contacts is stiffer in rotation than in translation), 0.15 is safe
	double timeStepFactor = 0.15;

	// Particles that leave the domain in x or z are removed (they fell off the bed); the ground y = 0 extends over the whole domain
	double minX = -5.0;
	double maxX = 105.0;
	double minZ = -5.0;
	double maxZ = 25.0;

	// Settling: a particle is at rest while its velocity stays below restVelocity, and settled when it has been at rest for restTime
	// (its rotation does not count: a particle turning in place moves no material, and its spin stops when it falls asleep). A settled particle falls asleep: it keeps supporting and touching its neighbors but is no longer
	// moved, which leaves the computing time to the material in motion. Any particle faster than wakeVelocity that touches a sleeping
	// one wakes it, so impacts and avalanches spread through the pile as before. A sleeping particle buried deeper than burialDepth
	// largest diameters below the lowest surface around it is fixed for good: material that deep carries the pile and does not move.
	double restVelocity = 0.05;
	double restTime = 0.5;
	double wakeVelocity = 0.1;
	double burialDepth = 3.0;
	// Interval in s of the bookkeeping: surface heights, fixing buried particles, removing particles off the bed
	double maintenanceInterval = 0.05;

	// Pairs are kept for particles closer than their radii plus skinFactor times the largest radius, and found again for a particle
	// once it moved by a quarter of that skin
	double skinFactor = 0.4;

	// Threads of the force computation, 0 for the OpenMP default; results do not depend on it
	int threads = 1;
};

// A discrete element method world of spheres on a flat ground. Particles are dynamic, sleeping (at rest, woken by moving neighbors),
// fixed (settled and buried) or removed (left the domain); sleeping and fixed particles still touch and support their neighbors. Forces
// are computed per dynamic particle from its own contacts, each contact from both sides, so the loop needs no synchronization and its
// result does not depend on the number of threads.
class World
{
	public:
		enum class State : std::uint8_t
		{
			Dynamic,
			Sleeping,
			Fixed,
			Removed,
		};

		World(const WorldSettings& settings, const Material& material, double minRadius, double maxRadius);

		// Adds a particle and returns its index; indices stay valid for the lifetime of the world
		int addParticle(const Vec3& position, const Vec3& velocity, double radius);

		// Advances by one time step
		void step();

		// Advances until the time is at least t
		void advanceTo(double t);

		double time() const { return currentTime; }
		double timeStep() const { return dt; }

		std::size_t particleCount() const { return radius.size(); }
		std::size_t dynamicCount() const { return dynamicIds.size(); }
		const Vec3& getPosition(int i) const { return position[i]; }
		const Vec3& getVelocity(int i) const { return velocity[i]; }
		const Vec3& getAngularVelocity(int i) const { return angularVelocity[i]; }
		double getRadius(int i) const { return radius[i]; }
		State getState(int i) const { return state[i]; }

		// Whether every dynamic particle has settled
		bool settled() const;

		// Highest top of the particles that are not in flight within distance d (horizontally) of (x, z); 0 on the empty ground
		double surfaceHeight(double x, double z, double d) const;

		// Whether a sphere at p with radius r would overlap a particle
		bool overlaps(const Vec3& p, double r) const;

		double kineticEnergy() const;

		// Where the time went, for tuning: seconds per phase of step(), and counts
		struct Profile
		{
			double pairCheck = 0.0; // finding the particles whose pairs are searched again, dropping their pairs (serial)
			double pairSearch = 0.0; // searching their neighbors (parallel)
			double pairMerge = 0.0; // adding the found pairs with their histories (serial)
			double entries = 0.0; // pairs per particle (serial)
			double forces = 0.0; // contact forces and integration (parallel)
			double wakes = 0.0; // waking sleeping particles (serial)
			double maintenance = 0.0; // surface, sleeping, fixing (serial)
			std::uint64_t steps = 0;
			std::uint64_t pairUpdates = 0;
			double dynamicSum = 0.0; // sum over the steps of the dynamic particles
			double pairSum = 0.0; // sum over the steps of the pairs
		};
		const Profile& getProfile() const { return profile; }

	private:
		struct RestingEntry
		{
			double y;
			int id;
		};

		// A pair of particles closer than their radii plus the skin: i is dynamic; j is dynamic with i < j, or resting (sleeping or
		// fixed). The history is that of the contact as seen from i.
		struct Pair
		{
			int i;
			int j;
			ContactHistory history;
		};

		// Forces of a pair in the current step
		struct PairForce
		{
			Vec3 force; // on i, -force on j
			Vec3 torqueI;
			Vec3 torqueJ;
		};

		WorldSettings settings;
		Material material;
		ContactModel contactModel;
		double maxRadius;
		double skin;
		double dt;
		double currentTime = 0.0;
		double nextMaintenance = 0.0;

		// Particle data, by index
		std::vector<Vec3> position;
		std::vector<Vec3> velocity;
		std::vector<Vec3> angularVelocity;
		std::vector<Vec3> force;
		std::vector<Vec3> torque;
		std::vector<double> radius;
		std::vector<double> mass;
		std::vector<double> inertia;
		std::vector<double> restingFor;
		std::vector<State> state;
		std::vector<ContactHistory> groundHistory;
		std::vector<Vec3> listPosition; // Position at the last update of the particle's pairs
		std::vector<std::uint8_t> needsPairs; // The particle's pairs must be found again before the next force computation
		std::vector<int> flaggedResting; // Particles that fell asleep or were removed since the last pair update

		std::vector<int> dynamicIds;

		// Pairs, their forces, and per dynamic particle (by index in dynamicIds) its pairs and sides (CSR)
		std::vector<Pair> pairs;
		std::vector<PairForce> pairForces;
		std::vector<int> pairStart;
		std::vector<int> pairEntries; // pair index * 2 + (1 if the particle is j)
		std::vector<int> dynamicSlot; // index in dynamicIds by particle, -1 if not dynamic
		std::vector<int> slottedIds; // the particles with a slot in dynamicSlot
		bool entriesValid = false;
		// Histories of the pairs dropped in a pair update, by pair key (kept between updates for its buckets)
		std::unordered_map<std::uint64_t, ContactHistory> keptHistories;

		// Columns of cell x cell in x and z: the sleeping and fixed particles of each column sorted by height, and the dynamic particles
		// sorted by column at the last pair update
		double cell;
		int columnsX;
		int columnsZ;
		std::vector<std::vector<RestingEntry>> restingColumns;
		std::vector<std::pair<int, int>> dynamicByColumn; // (column, id), sorted
		std::vector<int> sortedColumn; // column of each particle in dynamicByColumn when it was sorted
		bool dynamicSetChanged = true; // particles became dynamic or stopped being dynamic since dynamicByColumn was sorted
		// Surface height per column: top of the fixed particles, and of all others but those in flight at the last maintenance
		std::vector<double> fixedSurface;
		std::vector<double> surface;
		// Sleeping particles touched by fast ones in the last step, per thread
		std::vector<std::vector<int>> toWake;
		int threads;
		Profile profile;

		int columnIndex(double x, double z) const;
		void columnOf(double x, double z, int& cx, int& cz) const;
		void updatePairs();
		void buildEntries();
		void computePair(std::size_t p, std::vector<int>& wake);
		void sumForces(int slot);
		void integrate(int id);
		void maintain();
		void sleep(int id);
		void wake(int id);
		void fix(int id);
		void removeFromColumn(int id);
		ContactState particleContact(int i, int j, bool resting, double distance, const Vec3& d) const;
};
}

#endif
