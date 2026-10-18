// Small experiments with the DEM world and the detailed simulator, for development
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstring>

#include "BlendingSimulator/BlendingSimulatorDetailed.h"
#include "BlendingSimulator/ParticleParameters.h"
#include "BlendingSimulator/dem/World.h"

namespace bs = blendingsimulator;
namespace dem = blendingsimulator::dem;

static void restitution()
{
	for (double factor : {0.2, 0.1, 0.05}) {
		for (double e : {0.1, 0.3, 0.7}) {
			for (double v : {0.5, 2.0, 6.0}) {
				dem::Material material;
				material.restitution = e;
				dem::WorldSettings settings;
				settings.minX = settings.minZ = -20;
				settings.maxX = settings.maxZ = 20;
				settings.gravity = 0.0;
				settings.timeStepFactor = factor;
				dem::World world(settings, material, 0.5, 0.5);
				const int a = world.addParticle({-1.0, 5.0, 0.0}, {v, 0.0, 0.0}, 0.5);
				const int b = world.addParticle({1.0, 5.0, 0.0}, {-v, 0.0, 0.0}, 0.5);
				world.advanceTo(3.0);
				std::printf("factor %.2f e %.1f v %.1f -> %.3f\n", factor, e, v, (world.getVelocity(b).x - world.getVelocity(a).x) / (2.0 * v));
			}
		}
	}
}

static void settling()
{
	bs::SimulationParameters p;
	p.heapWorldSizeX = 10.0f;
	p.heapWorldSizeZ = 10.0f;
	p.particlesPerCubicMeter = 8.0f;
	p.seed = 1;
	bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(p);
	simulator.stack(4.0f, 5.0f, bs::AveragedParameters(6.0, {1.0}));
	simulator.stack(6.0f, 5.0f, bs::AveragedParameters(6.0, {3.0}));
	const dem::World& w = simulator.getWorld();
	auto& world = const_cast<dem::World&>(w);
	for (int k = 0; k < 40; k++) {
		world.advanceTo(world.time() + 0.5);
		double vmax = 0.0;
		double wmax = 0.0;
		int arg = -1;
		for (std::size_t i = 0; i < world.particleCount(); i++) {
			if (world.getState(int(i)) != dem::World::State::Dynamic) {
				continue;
			}
			const double v = dem::norm(world.getVelocity(int(i)));
			if (v > vmax) {
				vmax = v;
				arg = int(i);
			}
		}
		const dem::Vec3 p0 = arg >= 0 ? world.getPosition(arg) : dem::Vec3();
		std::printf("t %.1f dynamic %zu settled %d vmax %.4f at particle %d (%.2f %.2f %.2f) E %.3g\n", world.time(), world.dynamicCount(),
			world.settled(), vmax, arg, p0.x, p0.y, p0.z, world.kineticEnergy());
		(void)wmax;
	}
}

static void chevronTail()
{
	bs::SimulationParameters p;
	// the bed of bmh.helpers.bed_setup in blending-evaluation
	p.heapWorldSizeX = 59.0f;
	p.heapWorldSizeZ = 20.0f;
	p.particlesPerCubicMeter = 1.0f;
	p.seed = 1;
	p.detailed.threads = 4;
	bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(p);
	const int calls = 10000;
	for (int k = 0; k < calls; k++) {
		const double u = (k + 0.5) / calls * 19.0;
		const double frac = u - std::floor(u);
		const double x = 10.0 + 39.0 * (int(std::floor(u)) % 2 == 0 ? frac : 1.0 - frac);
		simulator.stack(float(x), 10.0f, bs::AveragedParameters(0.25, {0.0}));
	}
	auto& world = const_cast<dem::World&>(simulator.getWorld());
	world.advanceTo(world.time() + 20.0);
	int watch = -1;
	for (std::size_t i = 0; i < world.particleCount(); i++) {
		if (world.getState(int(i)) == dem::World::State::Dynamic) {
			watch = int(i);
			break;
		}
	}
	if (watch >= 0) {
		for (int k = 0; k < 400; k++) {
			world.step();
			if (k % 10 == 0) {
				const dem::Vec3 w = world.getAngularVelocity(watch);
				const dem::Vec3 v = world.getVelocity(watch);
				std::printf("watch %d step %d w (%.4f %.4f %.4f) |v| %.5f\n", watch, k, w.x, w.y, w.z, dem::norm(v));
			}
		}
	}
	for (int k = 0; k < 3; k++) {
		world.advanceTo(world.time() + 2.0);
		std::printf("t %.1f dynamic %zu settled %d E %.3g:", world.time(), world.dynamicCount(), world.settled(), world.kineticEnergy());
		int shown = 0;
		for (std::size_t i = 0; i < world.particleCount() && shown < 6; i++) {
			if (world.getState(int(i)) == dem::World::State::Dynamic) {
				const dem::Vec3 x = world.getPosition(int(i));
				const dem::Vec3 v = world.getVelocity(int(i));
				std::printf(" [%zu (%.2f %.2f %.2f) v %.4f]", i, x.x, x.y, x.z, dem::norm(v));
				shown++;
			}
		}
		std::printf("\n");
	}
}

static void centering()
{
	for (double speed : {0.0, 1.0}) {
		double sx = 0.0;
		double sz = 0.0;
		for (std::uint32_t seed = 1; seed <= 8; seed++) {
			bs::SimulationParameters p;
			p.heapWorldSizeX = 20.0f;
			p.heapWorldSizeZ = 20.0f;
			p.particlesPerCubicMeter = 8.0f;
			p.seed = seed;
			p.detailed.beltSpeed = speed;
			bs::BlendingSimulatorDetailed<bs::AveragedParameters> simulator(p);
			simulator.stack(10.0f, 10.0f, bs::AveragedParameters(30.0, {1.0}));
			simulator.finishStacking();
			const dem::World& world = simulator.getWorld();
			dem::Vec3 c;
			for (std::size_t i = 0; i < world.particleCount(); i++) {
				c += world.getPosition(int(i));
			}
			c = c / double(world.particleCount());
			std::printf("speed %.1f seed %u center %.3f %.3f\n", speed, seed, c.x, c.z);
			sx += c.x;
			sz += c.z;
		}
		std::printf("speed %.1f mean center %.3f %.3f\n", speed, sx / 8.0, sz / 8.0);
	}
}

int main(int argc, char** argv)
{
	const char* what = argc > 1 ? argv[1] : "restitution";
	if (std::strcmp(what, "restitution") == 0) {
		restitution();
	} else if (std::strcmp(what, "settling") == 0) {
		settling();
	} else if (std::strcmp(what, "centering") == 0) {
		centering();
	} else if (std::strcmp(what, "chevron") == 0) {
		chevronTail();
	}
	return 0;
}
