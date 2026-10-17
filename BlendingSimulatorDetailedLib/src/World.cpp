#include "BlendingSimulator/dem/World.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <unordered_map>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace blendingsimulator::dem
{
namespace
{
constexpr double pi = 3.141592653589793238463;

int threadCount(int requested)
{
#ifdef _OPENMP
	return requested > 0 ? requested : omp_get_max_threads();
#else
	(void)requested;
	return 1;
#endif
}

int threadNumber()
{
#ifdef _OPENMP
	return omp_get_thread_num();
#else
	return 0;
#endif
}

std::uint64_t pairKey(int a, int b)
{
	const auto low = std::uint64_t(std::uint32_t(std::min(a, b)));
	const auto high = std::uint64_t(std::uint32_t(std::max(a, b)));
	return (low << 32) | high;
}

ContactHistory negated(const ContactHistory& h)
{
	ContactHistory n = h;
	n.shear = -h.shear;
	n.rollingTorque = -h.rollingTorque;
	return n;
}

using Clock = std::chrono::steady_clock;

double seconds(Clock::time_point from, Clock::time_point to)
{
	return std::chrono::duration<double>(to - from).count();
}

// Loops below this size run on one thread: the threads cost more than they save
[[maybe_unused]] constexpr int parallelThreshold = 512;
}

World::World(const WorldSettings& settings, const Material& material, double minRadius, double maxRadius)
	: settings(settings)
	, material(material)
	, contactModel(material)
	, maxRadius(maxRadius)
	, skin(settings.skinFactor * maxRadius)
	, cell(2.0 * maxRadius + settings.skinFactor * maxRadius)
	, threads(threadCount(settings.threads))
{
	if (minRadius <= 0.0 || maxRadius < minRadius) {
		throw std::invalid_argument("particle radii must be positive with minRadius <= maxRadius");
	}
	if (settings.maxX <= settings.minX || settings.maxZ <= settings.minZ) {
		throw std::invalid_argument("the domain must have a positive extent");
	}

	// Rayleigh time of the smallest particle: the time a surface wave needs to cross it, the usual bound of the DEM time step
	const double shearModulus = material.youngsModulus / (2.0 * (1.0 + material.poissonRatio));
	const double rayleighTime = pi * minRadius * std::sqrt(material.density / shearModulus) / (0.1631 * material.poissonRatio + 0.8766);
	dt = settings.timeStepFactor * rayleighTime;

	columnsX = std::max(1, int(std::ceil((settings.maxX - settings.minX) / cell)));
	columnsZ = std::max(1, int(std::ceil((settings.maxZ - settings.minZ) / cell)));
	restingColumns.resize(std::size_t(columnsX) * columnsZ);
	fixedSurface.assign(restingColumns.size(), 0.0);
	surface.assign(restingColumns.size(), 0.0);
	toWake.resize(std::size_t(threads));
	nextMaintenance = settings.maintenanceInterval;
}

void World::columnOf(double x, double z, int& cx, int& cz) const
{
	cx = std::clamp(int(std::floor((x - settings.minX) / cell)), 0, columnsX - 1);
	cz = std::clamp(int(std::floor((z - settings.minZ) / cell)), 0, columnsZ - 1);
}

int World::columnIndex(double x, double z) const
{
	int cx;
	int cz;
	columnOf(x, z, cx, cz);
	return cz * columnsX + cx;
}

int World::addParticle(const Vec3& p, const Vec3& v, double r)
{
	if (r <= 0.0 || r > maxRadius * (1.0 + 1e-9)) {
		throw std::invalid_argument("particle radius out of the range given to the world");
	}
	const int id = int(radius.size());
	const double m = material.density * 4.0 / 3.0 * pi * r * r * r;
	position.push_back(p);
	velocity.push_back(v);
	angularVelocity.emplace_back();
	force.emplace_back();
	torque.emplace_back();
	radius.push_back(r);
	mass.push_back(m);
	inertia.push_back(0.4 * m * r * r);
	restingFor.push_back(0.0);
	state.push_back(State::Dynamic);
	groundHistory.emplace_back();
	listPosition.push_back(p);
	needsPairs.push_back(1);
	dynamicIds.push_back(id);
	dynamicSlot.push_back(-1);
	sortedColumn.push_back(-1);
	entriesValid = false;
	dynamicSetChanged = true;
	return id;
}

// Drops the pairs of the particles that moved by a quarter of the skin since their pairs were found, or changed their state, and finds
// their pairs again. Two particles that both moved less than a quarter of the skin since each was last searched, at whatever times, are
// still apart by more than the skin less half of it if they were not paired, so every contact is in the list.
void World::updatePairs()
{
	const double limit2 = 0.0625 * skin * skin;
	std::vector<int> search;
	for (int id : dynamicIds) {
		if (needsPairs[std::size_t(id)] || squaredNorm(position[std::size_t(id)] - listPosition[std::size_t(id)]) > limit2) {
			needsPairs[std::size_t(id)] = 1;
			search.push_back(id);
		}
	}
	if (search.empty() && flaggedResting.empty()) {
		return;
	}
	const Clock::time_point checkStart = Clock::now();
	profile.pairUpdates++;

	// Drop the pairs of every flagged particle (dynamic or not), keeping their histories as seen from the lower index. Most often the
	// only flagged particles are falling ones of the stream without any pair: then there is nothing to drop
	bool dropPass = !entriesValid || !flaggedResting.empty();
	for (std::size_t k = 0; k < search.size() && !dropPass; k++) {
		const int slot = dynamicSlot[std::size_t(search[k])];
		dropPass = slot < 0 || pairStart[std::size_t(slot) + 1] > pairStart[std::size_t(slot)];
	}
	std::unordered_map<std::uint64_t, ContactHistory>& kept = keptHistories;
	kept.clear();
	if (dropPass) {
		std::size_t out = 0;
		for (std::size_t p = 0; p < pairs.size(); p++) {
			const Pair& pair = pairs[p];
			if (needsPairs[std::size_t(pair.i)] || needsPairs[std::size_t(pair.j)]) {
				kept.emplace(pairKey(pair.i, pair.j), pair.i < pair.j ? pair.history : negated(pair.history));
			} else {
				pairs[out++] = pair;
			}
		}
		pairs.resize(out);
	}
	const std::size_t pairsBefore = pairs.size();

	// The dynamic particles sorted by column: sorted again only when one of them changed its column or the set changed (falling particles
	// of the stream mostly stay in their column)
	bool resort = dynamicSetChanged;
	for (std::size_t k = 0; k < dynamicIds.size() && !resort; k++) {
		const std::size_t id = std::size_t(dynamicIds[k]);
		resort = sortedColumn[id] != columnIndex(position[id].x, position[id].z);
	}
	if (resort) {
		dynamicByColumn.clear();
		for (int id : dynamicIds) {
			const int c = columnIndex(position[std::size_t(id)].x, position[std::size_t(id)].z);
			sortedColumn[std::size_t(id)] = c;
			dynamicByColumn.emplace_back(c, id);
		}
		std::sort(dynamicByColumn.begin(), dynamicByColumn.end());
		dynamicSetChanged = false;
	}

	auto addPair = [&](int i, int j) {
		Pair pair{i, j, ContactHistory()};
		pair.history.partner = j;
		if (auto it = kept.find(pairKey(i, j)); it != kept.end()) {
			pair.history = i < j ? it->second : negated(it->second);
			pair.history.partner = j;
		}
		pairs.push_back(pair);
	};

	const Clock::time_point searchStart = Clock::now();
	profile.pairCheck += seconds(checkStart, searchStart);
	// Search the neighbors of the flagged particles in parallel, each into its own list, and add the pairs in the order of the search
	std::vector<std::vector<std::pair<int, int>>> found(search.size());
	const int searchCount = int(search.size());
	const int t = threads;
	(void)t;
#pragma omp parallel for schedule(dynamic, 8) num_threads(t) if (t > 1 && searchCount > 64)
	for (int k = 0; k < searchCount; k++) {
		const int i = search[std::size_t(k)];
		std::vector<std::pair<int, int>>& out = found[std::size_t(k)];
		const Vec3& pi_ = position[std::size_t(i)];
		const double ri = radius[std::size_t(i)];
		int cx;
		int cz;
		columnOf(pi_.x, pi_.z, cx, cz);
		const double yLow = pi_.y - ri - maxRadius - skin;
		const double yHigh = pi_.y + ri + maxRadius + skin;
		for (int z = std::max(0, cz - 1); z <= std::min(columnsZ - 1, cz + 1); z++) {
			const int first = z * columnsX + std::max(0, cx - 1);
			const int last = z * columnsX + std::min(columnsX - 1, cx + 1);
			auto begin = std::lower_bound(dynamicByColumn.begin(), dynamicByColumn.end(), std::make_pair(first, std::numeric_limits<int>::min()));
			for (auto it = begin; it != dynamicByColumn.end() && it->first <= last; ++it) {
				const int j = it->second;
				// A pair of two searched particles is added once, by the lower index
				if (j == i || (needsPairs[std::size_t(j)] && j < i)) {
					continue;
				}
				const double reach = ri + radius[std::size_t(j)] + skin;
				if (squaredNorm(position[std::size_t(j)] - pi_) < reach * reach) {
					out.emplace_back(std::min(i, j), std::max(i, j));
				}
			}
			for (int c = first; c <= last; c++) {
				const std::vector<RestingEntry>& column = restingColumns[std::size_t(c)];
				auto from = std::lower_bound(column.begin(), column.end(), yLow, [](const RestingEntry& e, double y) { return e.y < y; });
				for (auto it = from; it != column.end() && it->y <= yHigh; ++it) {
					const int j = it->id;
					const double reach = ri + radius[std::size_t(j)] + skin;
					if (squaredNorm(position[std::size_t(j)] - pi_) < reach * reach) {
						out.emplace_back(i, j);
					}
				}
			}
		}
		listPosition[std::size_t(i)] = pi_;
	}
	const Clock::time_point mergeStart = Clock::now();
	profile.pairSearch += seconds(searchStart, mergeStart);
	for (const std::vector<std::pair<int, int>>& list : found) {
		for (const auto& [i, j] : list) {
			addPair(i, j);
		}
	}
	for (int id : search) {
		needsPairs[std::size_t(id)] = 0;
	}
	for (int id : flaggedResting) {
		needsPairs[std::size_t(id)] = 0;
	}
	flaggedResting.clear();
	if (dropPass || pairs.size() != pairsBefore) {
		entriesValid = false;
	}
	profile.pairMerge += seconds(mergeStart, Clock::now());
}

void World::buildEntries()
{
	for (int id : slottedIds) {
		dynamicSlot[std::size_t(id)] = -1;
	}
	for (std::size_t k = 0; k < dynamicIds.size(); k++) {
		dynamicSlot[std::size_t(dynamicIds[k])] = int(k);
	}
	slottedIds = dynamicIds;
	pairStart.assign(dynamicIds.size() + 1, 0);
	for (const Pair& pair : pairs) {
		pairStart[std::size_t(dynamicSlot[std::size_t(pair.i)]) + 1]++;
		if (state[std::size_t(pair.j)] == State::Dynamic) {
			pairStart[std::size_t(dynamicSlot[std::size_t(pair.j)]) + 1]++;
		}
	}
	for (std::size_t k = 0; k < dynamicIds.size(); k++) {
		pairStart[k + 1] += pairStart[k];
	}
	pairEntries.resize(std::size_t(pairStart.back()));
	std::vector<int> fill(pairStart.begin(), pairStart.end() - 1);
	for (std::size_t p = 0; p < pairs.size(); p++) {
		pairEntries[std::size_t(fill[std::size_t(dynamicSlot[std::size_t(pairs[p].i)])]++)] = int(2 * p);
		if (state[std::size_t(pairs[p].j)] == State::Dynamic) {
			pairEntries[std::size_t(fill[std::size_t(dynamicSlot[std::size_t(pairs[p].j)])]++)] = int(2 * p + 1);
		}
	}
	pairForces.resize(pairs.size());
	entriesValid = true;
}

ContactState World::particleContact(int i, int j, bool resting, double distance, const Vec3& d) const
{
	const std::size_t a = std::size_t(i);
	const std::size_t b = std::size_t(j);
	const double ri = radius[a];
	const double rj = radius[b];
	ContactState s;
	s.normal = d / distance;
	s.overlap = ri + rj - distance;
	s.radius = ri;
	s.effectiveRadius = ri * rj / (ri + rj);
	s.effectiveMass = resting ? mass[a] : mass[a] * mass[b] / (mass[a] + mass[b]);
	const Vec3 contactVelocityI = velocity[a] + cross(angularVelocity[a], s.normal * ri);
	const Vec3 contactVelocityJ = velocity[b] + cross(angularVelocity[b], s.normal * -rj);
	s.relativeVelocity = contactVelocityI - contactVelocityJ;
	s.relativeAngularVelocity = angularVelocity[a] - angularVelocity[b];
	const double rollingI = inertia[a] + mass[a] * ri * ri;
	const double rollingJ = inertia[b] + mass[b] * rj * rj;
	s.rollingInertia = resting ? rollingI : 1.0 / (1.0 / rollingI + 1.0 / rollingJ);
	s.friction = material.friction;
	s.rollingFriction = material.rollingFriction;
	return s;
}

void World::computePair(std::size_t p, std::vector<int>& wake)
{
	Pair& pair = pairs[p];
	PairForce& out = pairForces[p];
	const Vec3 d = position[std::size_t(pair.j)] - position[std::size_t(pair.i)];
	const double reach = radius[std::size_t(pair.i)] + radius[std::size_t(pair.j)];
	const double distance2 = squaredNorm(d);
	if (distance2 >= reach * reach || distance2 == 0.0) {
		// Not (or no longer) in contact: the contact's history ends
		pair.history.shear = {};
		pair.history.rollingTorque = {};
		out = PairForce();
		return;
	}
	const bool resting = state[std::size_t(pair.j)] != State::Dynamic;
	const double distance = std::sqrt(distance2);
	const ContactState s = particleContact(pair.i, pair.j, resting, distance, d);
	const ContactForce c = contactModel.compute(s, pair.history, dt);
	out.force = c.force;
	out.torqueI = c.torque;
	// The partner sees the negated normal, tangential force and rolling torque
	out.torqueJ = cross(s.normal * radius[std::size_t(pair.j)], c.tangentialForce) - c.rollingTorque;

	if (state[std::size_t(pair.j)] == State::Sleeping) {
		const double wake2 = settings.wakeVelocity * settings.wakeVelocity;
		const std::size_t i = std::size_t(pair.i);
		if (squaredNorm(velocity[i]) > wake2 || squaredNorm(angularVelocity[i]) * radius[i] * radius[i] > wake2) {
			wake.push_back(pair.j);
		}
	}
}

void World::sumForces(int slot)
{
	const std::size_t id = std::size_t(dynamicIds[std::size_t(slot)]);
	Vec3 f;
	Vec3 t;

	// Ground
	const double r = radius[id];
	if (position[id].y < r) {
		ContactState s;
		s.normal = {0.0, -1.0, 0.0};
		s.overlap = r - position[id].y;
		s.radius = r;
		s.effectiveRadius = r;
		s.effectiveMass = mass[id];
		s.relativeVelocity = velocity[id] + cross(angularVelocity[id], s.normal * r);
		s.relativeAngularVelocity = angularVelocity[id];
		s.rollingInertia = inertia[id] + mass[id] * r * r;
		s.friction = material.groundFriction;
		s.rollingFriction = material.groundRollingFriction;
		const ContactForce c = contactModel.compute(s, groundHistory[id], dt);
		f += c.force;
		t += c.torque;
	} else {
		groundHistory[id] = ContactHistory();
	}

	for (int e = pairStart[std::size_t(slot)]; e < pairStart[std::size_t(slot) + 1]; e++) {
		const int entry = pairEntries[std::size_t(e)];
		const PairForce& pf = pairForces[std::size_t(entry >> 1)];
		if (entry & 1) {
			f -= pf.force;
			t += pf.torqueJ;
		} else {
			f += pf.force;
			t += pf.torqueI;
		}
	}
	force[id] = f;
	torque[id] = t;
}

void World::integrate(int i)
{
	const std::size_t k = std::size_t(i);
	// Semi-implicit (symplectic) Euler
	velocity[k] += (force[k] / mass[k] + Vec3(0.0, -settings.gravity, 0.0)) * dt;
	position[k] += velocity[k] * dt;
	angularVelocity[k] += torque[k] * (dt / inertia[k]);

	// At rest by its translation: a particle turning in place, as one caged among resting neighbors may do, moves no material
	if (squaredNorm(velocity[k]) < settings.restVelocity * settings.restVelocity) {
		restingFor[k] += dt;
	} else {
		restingFor[k] = 0.0;
	}
}

void World::step()
{
	const Clock::time_point start = Clock::now();
	updatePairs();
	const Clock::time_point paired = Clock::now();
	if (!entriesValid) {
		buildEntries();
	}
	const Clock::time_point entered = Clock::now();
	profile.entries += seconds(paired, entered);

	const int pairCount = int(pairs.size());
	const int n = int(dynamicIds.size());
	const int t = threads;
	(void)t;
#pragma omp parallel num_threads(t) if (t > 1 && pairCount + n > parallelThreshold)
	{
#pragma omp for schedule(static)
		for (int p = 0; p < pairCount; p++) {
			computePair(std::size_t(p), toWake[std::size_t(threadNumber())]);
		}
#pragma omp for schedule(static)
		for (int k = 0; k < n; k++) {
			sumForces(k);
			integrate(dynamicIds[std::size_t(k)]);
		}
	}

	const Clock::time_point computed = Clock::now();
	profile.forces += seconds(entered, computed);
	profile.steps++;
	profile.dynamicSum += double(n);
	profile.pairSum += double(pairCount);
	(void)start;

	// Wake the sleeping particles that fast ones touched, in index order so that the result does not depend on the threads
	std::vector<int> woken;
	for (std::vector<int>& list : toWake) {
		woken.insert(woken.end(), list.begin(), list.end());
		list.clear();
	}
	if (!woken.empty()) {
		std::sort(woken.begin(), woken.end());
		woken.erase(std::unique(woken.begin(), woken.end()), woken.end());
		for (int id : woken) {
			wake(id);
		}
		dynamicIds.insert(dynamicIds.end(), woken.begin(), woken.end());
		std::sort(dynamicIds.begin(), dynamicIds.end());
		entriesValid = false;
		dynamicSetChanged = true;
	}

	const Clock::time_point awake = Clock::now();
	profile.wakes += seconds(computed, awake);

	currentTime += dt;
	if (currentTime >= nextMaintenance) {
		maintain();
		nextMaintenance = currentTime + settings.maintenanceInterval;
		profile.maintenance += seconds(awake, Clock::now());
	}
}

void World::advanceTo(double t)
{
	while (currentTime < t) {
		step();
	}
}

void World::removeFromColumn(int id)
{
	std::vector<RestingEntry>& column = restingColumns[std::size_t(columnIndex(position[std::size_t(id)].x, position[std::size_t(id)].z))];
	column.erase(std::find_if(column.begin(), column.end(), [id](const RestingEntry& e) { return e.id == id; }));
}

void World::sleep(int id)
{
	const std::size_t k = std::size_t(id);
	state[k] = State::Sleeping;
	velocity[k] = {};
	angularVelocity[k] = {};
	std::vector<RestingEntry>& column = restingColumns[std::size_t(columnIndex(position[k].x, position[k].z))];
	const double y = position[k].y;
	auto at = std::upper_bound(column.begin(), column.end(), y, [](double v, const RestingEntry& e) { return v < e.y; });
	column.insert(at, {y, id});
	needsPairs[k] = 1;
	flaggedResting.push_back(id);
}

void World::wake(int id)
{
	removeFromColumn(id);
	const std::size_t k = std::size_t(id);
	state[k] = State::Dynamic;
	restingFor[k] = 0.0;
	needsPairs[k] = 1;
	listPosition[k] = position[k];
}

void World::fix(int id)
{
	state[std::size_t(id)] = State::Fixed;
	const std::size_t c = std::size_t(columnIndex(position[std::size_t(id)].x, position[std::size_t(id)].z));
	fixedSurface[c] = std::max(fixedSurface[c], position[std::size_t(id)].y + radius[std::size_t(id)]);
}

void World::maintain()
{
	// Surface: everything but the particles in flight
	surface = fixedSurface;
	const double slow2 = 100.0 * settings.restVelocity * settings.restVelocity;
	for (std::size_t id = 0; id < state.size(); id++) {
		if (state[id] == State::Sleeping || (state[id] == State::Dynamic && squaredNorm(velocity[id]) < slow2)) {
			const std::size_t c = std::size_t(columnIndex(position[id].x, position[id].z));
			surface[c] = std::max(surface[c], position[id].y + radius[id]);
		}
	}

	// Dynamic particles leave the domain or fall asleep; the dynamic particles paired with them search their pairs again (the pairs
	// of a particle that fell asleep are now pairs with a resting partner, those of a removed particle are gone)
	std::vector<int> left;
	for (int id : dynamicIds) {
		const std::size_t k = std::size_t(id);
		const Vec3& p = position[k];
		if (p.x < settings.minX || p.x > settings.maxX || p.z < settings.minZ || p.z > settings.maxZ || p.y < -10.0 * maxRadius) {
			state[k] = State::Removed;
			needsPairs[k] = 1;
			flaggedResting.push_back(id);
			left.push_back(id);
		} else if (restingFor[k] >= settings.restTime) {
			sleep(id);
			left.push_back(id);
		}
	}
	if (!left.empty()) {
		for (const Pair& pair : pairs) {
			if (needsPairs[std::size_t(pair.i)] && state[std::size_t(pair.j)] == State::Dynamic) {
				needsPairs[std::size_t(pair.j)] = 1;
			}
			if (needsPairs[std::size_t(pair.j)] && state[std::size_t(pair.i)] == State::Dynamic) {
				needsPairs[std::size_t(pair.i)] = 1;
			}
		}
		dynamicIds.erase(std::remove_if(dynamicIds.begin(), dynamicIds.end(), [this](int id) { return state[std::size_t(id)] != State::Dynamic; }),
			dynamicIds.end());
		entriesValid = false;
		dynamicSetChanged = true;
	}

	// Sleeping particles buried deep enough are fixed for good
	const double depth = settings.burialDepth * 2.0 * maxRadius;
	for (std::size_t id = 0; id < state.size(); id++) {
		if (state[id] != State::Sleeping) {
			continue;
		}
		int cx;
		int cz;
		columnOf(position[id].x, position[id].z, cx, cz);
		double lowest = std::numeric_limits<double>::infinity();
		for (int z = std::max(0, cz - 1); z <= std::min(columnsZ - 1, cz + 1); z++) {
			for (int x = std::max(0, cx - 1); x <= std::min(columnsX - 1, cx + 1); x++) {
				lowest = std::min(lowest, surface[std::size_t(z) * columnsX + x]);
			}
		}
		if (position[id].y + radius[id] < lowest - depth) {
			fix(int(id));
		}
	}
}

bool World::settled() const
{
	for (int id : dynamicIds) {
		if (restingFor[std::size_t(id)] < settings.restTime) {
			return false;
		}
	}
	return true;
}

double World::surfaceHeight(double x, double z, double d) const
{
	int cx0;
	int cz0;
	int cx1;
	int cz1;
	columnOf(x - d, z - d, cx0, cz0);
	columnOf(x + d, z + d, cx1, cz1);
	double h = 0.0;
	for (int cz = cz0; cz <= cz1; cz++) {
		for (int cx = cx0; cx <= cx1; cx++) {
			h = std::max(h, surface[std::size_t(cz) * columnsX + cx]);
		}
	}
	return h;
}

bool World::overlaps(const Vec3& p, double r) const
{
	for (int id : dynamicIds) {
		const double reach = r + radius[std::size_t(id)];
		if (squaredNorm(position[std::size_t(id)] - p) < reach * reach) {
			return true;
		}
	}
	int cx;
	int cz;
	columnOf(p.x, p.z, cx, cz);
	for (int z = std::max(0, cz - 1); z <= std::min(columnsZ - 1, cz + 1); z++) {
		for (int x = std::max(0, cx - 1); x <= std::min(columnsX - 1, cx + 1); x++) {
			const std::vector<RestingEntry>& column = restingColumns[std::size_t(z) * columnsX + x];
			auto from = std::lower_bound(column.begin(), column.end(), p.y - r - maxRadius, [](const RestingEntry& e, double y) { return e.y < y; });
			for (auto it = from; it != column.end() && it->y <= p.y + r + maxRadius; ++it) {
				const double reach = r + radius[std::size_t(it->id)];
				if (squaredNorm(position[std::size_t(it->id)] - p) < reach * reach) {
					return true;
				}
			}
		}
	}
	return false;
}

double World::kineticEnergy() const
{
	double e = 0.0;
	for (int id : dynamicIds) {
		const std::size_t k = std::size_t(id);
		e += 0.5 * mass[k] * squaredNorm(velocity[k]) + 0.5 * inertia[k] * squaredNorm(angularVelocity[k]);
	}
	return e;
}
}
