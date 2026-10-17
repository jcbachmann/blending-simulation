#include <gtest/gtest.h>

#include <cmath>

#include "BlendingSimulator/BlendingSimulatorDetailed.h"
#include "BlendingSimulator/ParticleParameters.h"
#include "BlendingSimulator/dem/World.h"

namespace bs = blendingsimulator;
namespace dem = blendingsimulator::dem;

namespace
{
constexpr double pi = 3.141592653589793238463;

dem::WorldSettings smallDomain()
{
	dem::WorldSettings settings;
	settings.minX = -10.0;
	settings.maxX = 10.0;
	settings.minZ = -10.0;
	settings.maxZ = 10.0;
	return settings;
}

bs::SimulationParameters detailedParameters(float sizeX, float sizeZ, float ppm3)
{
	bs::SimulationParameters p;
	p.heapWorldSizeX = sizeX;
	p.heapWorldSizeZ = sizeZ;
	p.reclaimAngle = 45.0f;
	p.particlesPerCubicMeter = ppm3;
	p.seed = 1;
	return p;
}
}

// A sphere resting on the ground compresses it by the Hertz overlap of its weight: m g = 4/3 E* sqrt(r) d^(3/2)
TEST(DemWorld, test_resting_overlap_is_hertzian)
{
	dem::Material material;
	dem::World world(smallDomain(), material, 0.5, 0.5);
	const int id = world.addParticle({0.0, 0.5, 0.0}, {}, 0.5);
	world.advanceTo(5.0);

	const double mass = material.density * 4.0 / 3.0 * pi * 0.125;
	const dem::ContactModel model(material);
	const double expected = std::pow(mass * 9.80665 / (4.0 / 3.0 * model.effectiveYoungsModulus() * std::sqrt(0.5)), 2.0 / 3.0);
	EXPECT_NEAR(0.5 - world.getPosition(id).y, expected, 0.01 * expected);
	EXPECT_LT(std::abs(world.getVelocity(id).y), 1e-6);
}

// Head-on impact without gravity: the ratio of the speeds after and before is the coefficient of restitution (resolved with half the
// default time step; the default resolves impacts of a few m/s within about 10 %)
TEST(DemWorld, test_restitution)
{
	for (double e : {0.1, 0.3, 0.7}) {
		dem::Material material;
		material.restitution = e;
		dem::WorldSettings settings = smallDomain();
		settings.gravity = 0.0;
		settings.timeStepFactor = 0.1;
		dem::World world(settings, material, 0.5, 0.5);
		const int a = world.addParticle({-1.0, 5.0, 0.0}, {1.0, 0.0, 0.0}, 0.5);
		const int b = world.addParticle({1.0, 5.0, 0.0}, {-1.0, 0.0, 0.0}, 0.5);
		world.advanceTo(2.0);
		EXPECT_NEAR(world.getVelocity(b).x - world.getVelocity(a).x, 2.0 * e, 2.0 * 0.01) << "e = " << e;
	}
}

// Both sides of a contact compute exactly opposite forces: the momentum of a cluster of colliding particles is conserved
TEST(DemWorld, test_momentum_conservation)
{
	dem::WorldSettings settings = smallDomain();
	settings.gravity = 0.0;
	dem::World world(settings, dem::Material(), 0.4, 0.5);
	const std::vector<dem::Vec3> starts = {{0.0, 5.0, 0.0}, {0.85, 5.1, 0.1}, {0.4, 5.8, -0.2}, {-0.5, 5.6, 0.6}};
	const std::vector<dem::Vec3> velocities = {{1.0, 0.0, 0.0}, {-1.0, 0.3, 0.0}, {0.0, -2.0, 0.5}, {0.7, -0.2, -0.9}};
	dem::Vec3 before;
	for (std::size_t i = 0; i < starts.size(); i++) {
		const double r = 0.4 + 0.03 * double(i);
		world.addParticle(starts[i], velocities[i], r);
		before += velocities[i] * (r * r * r);
	}
	world.advanceTo(1.0);
	dem::Vec3 after;
	for (std::size_t i = 0; i < starts.size(); i++) {
		const double r = 0.4 + 0.03 * double(i);
		after += world.getVelocity(int(i)) * (r * r * r);
	}
	EXPECT_NEAR(after.x, before.x, 1e-12);
	EXPECT_NEAR(after.y, before.y, 1e-12);
	EXPECT_NEAR(after.z, before.z, 1e-12);
}

// A sphere rolling on the ground comes to rest through rolling resistance
TEST(DemWorld, test_rolling_resistance_stops_rolling)
{
	dem::World world(smallDomain(), dem::Material(), 0.5, 0.5);
	const int id = world.addParticle({0.0, 0.5, 0.0}, {2.0, 0.0, 0.0}, 0.5);
	world.advanceTo(8.0);
	EXPECT_LT(dem::norm(world.getVelocity(id)), 1e-3);
	EXPECT_GT(world.getPosition(id).x, 0.1);
	EXPECT_LT(world.getPosition(id).x, 9.0);
}

// The forces of each particle are computed independently, so the number of threads does not change the result
TEST(DemWorld, test_threads_give_identical_results)
{
	auto run = [](int threads) {
		dem::WorldSettings settings = smallDomain();
		settings.threads = threads;
		dem::World world(settings, dem::Material(), 0.2, 0.25);
		int k = 0;
		for (int i = 0; i < 12; i++) {
			for (int j = 0; j < 12; j++) {
				for (int l = 0; l < 4; l++, k++) {
					world.addParticle({0.55 * i - 3.0, 0.3 + 0.55 * l, 0.55 * j - 3.0}, {0.1 * (k % 3), 0.0, -0.1 * (k % 5)}, 0.2 + 0.0005 * (k % 100));
				}
			}
		}
		world.advanceTo(0.5);
		std::vector<double> state;
		for (std::size_t i = 0; i < world.particleCount(); i++) {
			state.push_back(world.getPosition(int(i)).x);
			state.push_back(world.getPosition(int(i)).y);
			state.push_back(world.getVelocity(int(i)).z);
		}
		return state;
	};
	EXPECT_EQ(run(1), run(4));
}

TEST(BlendingSimulatorDetailed, test_constructor_destructor)
{
	bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(detailedParameters(1.0f, 1.0f, 1.0f));
	const std::pair<float, float> heapWorldSize = simulator.getHeapWorldSize();
	EXPECT_NEAR(heapWorldSize.first, 1.0, 1e-6);
	EXPECT_NEAR(heapWorldSize.second, 1.0, 1e-6);
}

TEST(BlendingSimulatorDetailed, test_is_paused)
{
	bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(detailedParameters(1.0f, 1.0f, 1.0f));
	EXPECT_FALSE(simulator.isPaused());
	simulator.pause();
	EXPECT_TRUE(simulator.isPaused());
	simulator.resume();
	EXPECT_FALSE(simulator.isPaused());
}

TEST(BlendingSimulatorDetailed, test_heap_map_empty)
{
	bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(detailedParameters(10.0f, 20.0f, 1.0f));
	const auto size = simulator.getHeapMapSize();
	const float* heapMap = simulator.getHeapMap();
	ASSERT_NE(heapMap, nullptr);
	for (unsigned int i = 0; i < size.first * size.second; i++) {
		EXPECT_EQ(heapMap[i], 0.0f);
	}
}

TEST(BlendingSimulatorDetailed, test_stack_clear)
{
	bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(detailedParameters(6.0f, 6.0f, 1.0f));
	simulator.stack(3.0f, 3.0f, bs::AveragedParameters(5.0, {1.0}));
	simulator.finishStacking();
	EXPECT_EQ(simulator.getWorld().particleCount(), 5u);
	simulator.clear();
	EXPECT_EQ(simulator.getWorld().particleCount(), 0u);
	const auto size = simulator.getHeapMapSize();
	const float* heapMap = simulator.getHeapMap();
	for (unsigned int i = 0; i < size.first * size.second; i++) {
		EXPECT_EQ(heapMap[i], 0.0f);
	}
}

// The reclaimer takes up exactly the stacked volume, with its parameters
TEST(BlendingSimulatorDetailed, test_stack_reclaim_conserves_volume)
{
	bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(detailedParameters(10.0f, 10.0f, 8.0f));
	simulator.stack(4.0f, 5.0f, bs::AveragedParameters(6.0, {1.0}));
	simulator.stack(6.0f, 5.0f, bs::AveragedParameters(6.0, {3.0}));
	simulator.finishStacking();
	EXPECT_TRUE(simulator.getWorld().settled());
	EXPECT_NEAR(simulator.getLostVolume(), 0.0, 1e-12);

	EXPECT_FALSE(simulator.reclaimingFinished());
	bs::AveragedParameters total;
	for (float position = 0.0f; !simulator.reclaimingFinished(); position += 1.0f) {
		total.push(simulator.reclaim(position));
	}
	EXPECT_NEAR(total.getVolume(), 12.0, 1e-9);
	EXPECT_NEAR(total.getValues()[0], 2.0, 1e-9);
}

// Particle radii follow the packing fraction and stay within the spread
TEST(BlendingSimulatorDetailed, test_particle_sizes)
{
	bs::SimulationParameters p = detailedParameters(10.0f, 10.0f, 8.0f);
	bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(p);
	simulator.stack(5.0f, 5.0f, bs::AveragedParameters(4.0, {1.0}));
	const dem::World& world = simulator.getWorld();
	ASSERT_EQ(world.particleCount(), 32u);
	const double s = p.detailed.sizeSpread;
	const double r0 = std::cbrt(3.0 * p.detailed.packingFraction / 8.0 / (4.0 * pi * (1.0 + s * s)));
	EXPECT_NEAR(simulator.getMeanRadius(), r0, 1e-7);
	for (std::size_t i = 0; i < world.particleCount(); i++) {
		EXPECT_GE(world.getRadius(int(i)), r0 * (1.0 - s) - 1e-7);
		EXPECT_LE(world.getRadius(int(i)), r0 * (1.0 + s) + 1e-7);
	}
}

std::vector<double> stackWithSeed(std::uint32_t seed)
{
	bs::SimulationParameters p = detailedParameters(10.0f, 10.0f, 1.0f);
	p.seed = seed;
	bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(p);
	simulator.stack(5.0f, 5.0f, bs::AveragedParameters(5.0, {1.0}));
	simulator.finishStacking();
	std::vector<double> positions;
	const dem::World& world = simulator.getWorld();
	for (std::size_t i = 0; i < world.particleCount(); i++) {
		positions.push_back(world.getPosition(int(i)).x);
		positions.push_back(world.getPosition(int(i)).y);
		positions.push_back(world.getPosition(int(i)).z);
	}
	return positions;
}

TEST(BlendingSimulatorDetailed, test_seed_repeatable)
{
	const auto positions = stackWithSeed(42);
	EXPECT_EQ(positions.size(), 15u);
	EXPECT_EQ(positions, stackWithSeed(42));
}

TEST(BlendingSimulatorDetailed, test_seed_different)
{
	EXPECT_NE(stackWithSeed(1), stackWithSeed(2));
}

// The stream lands at the stacker position: a pile poured at one point with a vertical discharge is centered there, while a horizontal
// throw carries material further in the direction of the throw (+z)
dem::Vec3 pileCenter(double beltSpeed)
{
	bs::SimulationParameters p = detailedParameters(20.0f, 20.0f, 8.0f);
	p.detailed.beltSpeed = beltSpeed;
	bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(p);
	simulator.stack(10.0f, 10.0f, bs::AveragedParameters(30.0, {1.0}));
	simulator.finishStacking();
	const dem::World& world = simulator.getWorld();
	dem::Vec3 center;
	double top = 0.0;
	for (std::size_t i = 0; i < world.particleCount(); i++) {
		center += world.getPosition(int(i));
		top = std::max(top, world.getPosition(int(i)).y);
	}
	EXPECT_GT(top, 1.0); // a pile, not a layer
	return center / double(world.particleCount());
}

// The center of mass of a 240-particle pile scatters by about 0.1 m between seeds (mean over 8 seeds: 10.008, 10.011); a throw of
// 1 m/s moves it by 0.39 m on average
TEST(BlendingSimulatorDetailed, test_pile_centered_at_stacker)
{
	const dem::Vec3 vertical = pileCenter(0.0);
	EXPECT_NEAR(vertical.x, 10.0, 0.3);
	EXPECT_NEAR(vertical.z, 10.0, 0.3);

	const dem::Vec3 thrown = pileCenter(2.0);
	EXPECT_NEAR(thrown.x, 10.0, 0.3);
	EXPECT_GT(thrown.z, 10.3);
}

// Pausing the stacker (finishStacking while more is stacked afterwards) must not release the material that follows all at once
TEST(BlendingSimulatorDetailed, test_pauses_keep_all_material)
{
	bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(detailedParameters(20.0f, 20.0f, 8.0f));
	for (int k = 0; k < 6; k++) {
		simulator.stack(10.0f, 10.0f, bs::AveragedParameters(5.0, {double(k)}));
		simulator.finishStacking();
	}
	EXPECT_NEAR(simulator.getLostVolume(), 0.0, 1e-12);
	bs::AveragedParameters total;
	for (float position = 0.0f; !simulator.reclaimingFinished(); position += 1.0f) {
		total.push(simulator.reclaim(position));
	}
	EXPECT_NEAR(total.getVolume(), 30.0, 1e-9);
}

// The discharge delivers the flow at fine resolutions too: 10 m³ of 0.25 m particles (640) poured at one point take 10 s at 1 m³/s
TEST(BlendingSimulatorDetailed, test_stream_keeps_the_flow_rate)
{
	bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(detailedParameters(20.0f, 20.0f, 64.0f));
	simulator.stack(10.0f, 10.0f, bs::AveragedParameters(10.0, {1.0}));
	EXPECT_LT(simulator.getWorld().time(), 10.0 * 1.01);
	EXPECT_LT(simulator.getBlockedTime(), 0.1);
	simulator.finishStacking();
	EXPECT_NEAR(simulator.getLostVolume(), 0.0, 1e-12);
}
