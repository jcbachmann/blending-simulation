#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "BlendingSimulator/BlendingSimulatorCone.h"
#include "BlendingSimulator/ContinuousDrops.h"
#include "BlendingSimulator/ParticleParameters.h"

namespace bs = blendingsimulator;
using Cone = bs::BlendingSimulatorCone<bs::AveragedParameters>;

const double pi = std::atan(1.0) * 4.0;

bs::SimulationParameters coneParameters(float sizeX, float sizeZ, float particlesPerCubicMeter, float reclaimAngle = 45.0f)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = sizeX;
	simulationParameters.heapWorldSizeZ = sizeZ;
	simulationParameters.reclaimAngle = reclaimAngle;
	simulationParameters.particlesPerCubicMeter = particlesPerCubicMeter;
	return simulationParameters;
}

// Reclaims everything in steps of 1 m like the Python bindings: volumes and the values of the first parameter per slice
struct Reclaimed
{
	std::vector<double> volume;
	std::vector<std::vector<double>> values;

	double total() const
	{
		double sum = 0.0;
		for (double v : volume) {
			sum += v;
		}
		return sum;
	}
};

Reclaimed reclaimAll(bs::BlendingSimulator<bs::AveragedParameters>& simulator)
{
	Reclaimed result;
	float position = 0.0f;
	while (!simulator.reclaimingFinished()) {
		const bs::AveragedParameters slice = simulator.reclaim(position);
		result.volume.push_back(slice.getVolume());
		result.values.push_back(slice.getValues());
		position += 1.0f;
	}
	return result;
}

// A chevron of 10 passes over the core of a 59 m by 20 m bed, one portion per meter of stacker travel, quality rising over time
void stackChevron(Cone& simulator, double totalVolume)
{
	const int passes = 10;
	const int steps = 39;
	const double portion = totalVolume / (passes * steps);
	for (int pass = 0; pass < passes; pass++) {
		for (int s = 0; s < steps; s++) {
			const double x = pass % 2 == 0 ? 10.5 + s : 48.5 - s;
			const double quality = double(pass * steps + s) / (passes * steps);
			simulator.stack(float(x), 10.0f, bs::AveragedParameters(portion, {quality, 1.0 - quality}));
		}
	}
}

TEST(BlendingSimulatorCone, ConservesVolumeAndParameters)
{
	for (float ppm3 : {1.0f, 8.0f, 64.0f}) {
		Cone simulator(coneParameters(59.0f, 20.0f, ppm3));
		stackChevron(simulator, 2500.0);
		const Reclaimed reclaimed = reclaimAll(simulator);
		EXPECT_NEAR(reclaimed.total(), 2500.0, 1e-6) << "ppm3 " << ppm3;
		// Both parameters are carried along: their volume-weighted sum stays 1 in every slice
		for (std::size_t k = 0; k < reclaimed.volume.size(); k++) {
			if (reclaimed.volume[k] > 1e-9) {
				EXPECT_NEAR(reclaimed.values[k][0] + reclaimed.values[k][1], 1.0, 1e-9);
			}
		}
	}
}

TEST(BlendingSimulatorCone, SingleConeHasTheAnalyticApex)
{
	// On the empty bed a portion of volume V forms a cone of height cbrt(3 V / pi) at 45 degrees
	const double volume = 100.0;
	const double expected = std::cbrt(3.0 * volume / pi);
	for (float ppm3 : {8.0f, 64.0f, 512.0f}) {
		Cone simulator(coneParameters(30.0f, 30.0f, ppm3));
		simulator.stack(15.0f, 15.0f, bs::AveragedParameters(volume, {1.0}));
		// The apex lies between cell centers: the highest cell is within one cell size (at slope 1) below it
		const double top = simulator.getHeight(15.0, 15.0);
		EXPECT_LE(top, expected + 1e-9) << "ppm3 " << ppm3;
		EXPECT_GE(top, expected - 1.5 * std::max(simulator.getCellSizeX(), simulator.getCellSizeZ())) << "ppm3 " << ppm3;
		EXPECT_NEAR(reclaimAll(simulator).total(), volume, 1e-6);
	}
}

TEST(BlendingSimulatorCone, PilesHaveTheAngleOfRepose)
{
	for (double angle : {30.0, 35.0, 45.0}) {
		Cone simulator(coneParameters(40.0f, 40.0f, 64.0f), angle);
		simulator.stack(20.0f, 20.0f, bs::AveragedParameters(300.0, {1.0}));
		// The surface is exact at the cell centers: height against the distance to the drop point, fitted over the flanks of the pile, has
		// the slope of the angle of repose in every direction (fitted separately near the axes and near the diagonals)
		const double slope = std::tan(angle * pi / 180.0);
		const double cx = simulator.getCellSizeX(), cz = simulator.getCellSizeZ();
		double peak = 0.0;
		for (double x = 0.5 * cx; x < 40.0; x += cx) {
			for (double z = 0.5 * cz; z < 40.0; z += cz) {
				peak = std::max(peak, simulator.getHeight(x, z));
			}
		}
		for (bool diagonal : {false, true}) {
			std::vector<std::pair<double, double>> points;
			for (double x = 0.5 * cx; x < 40.0; x += cx) {
				for (double z = 0.5 * cz; z < 40.0; z += cz) {
					const double dx = x - 20.0, dz = z - 20.0;
					const double direction = std::fmod(std::abs(std::atan2(dz, dx)) * 4.0 / pi, 2.0);  // 0 on the axes, 1 on the diagonals
					if ((diagonal ? std::abs(direction - 1.0) : std::min(direction, 2.0 - direction)) < 0.2) {
						points.emplace_back(std::hypot(dx, dz), simulator.getHeight(x, z));
					}
				}
			}
			double n = 0.0, sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
			for (const auto& [d, h] : points) {
				if (h > 0.2 * peak && h < 0.8 * peak) {
					n += 1.0;
					sx += d;
					sy += h;
					sxx += d * d;
					sxy += d * h;
				}
			}
			EXPECT_NEAR(-(n * sxy - sx * sy) / (n * sxx - sx * sx), slope, 1e-6 * slope) << angle << (diagonal ? " diagonal" : " axis");
		}
	}
}

TEST(BlendingSimulatorCone, IsDeterministicAndMirrorSymmetric)
{
	auto run = [](bool mirrored) {
		// ppm3 4: cells of about 0.63 m, adjusted to tile the 59 m bed, so that mirroring maps cells onto cells
		Cone simulator(coneParameters(59.0f, 20.0f, 4.0f));
		const double xs[] = {12.3, 30.1, 47.7, 25.5, 18.2, 40.4};
		for (double x : xs) {
			simulator.stack(float(mirrored ? 59.0 - x : x), 10.0f, bs::AveragedParameters(60.0, {1.0}));
		}
		std::vector<double> profile;
		const double cx = simulator.getCellSizeX();
		for (double x = 0.5 * cx; x < 59.0; x += cx) {
			profile.push_back(simulator.getHeight(x, 10.0 + 0.5 * simulator.getCellSizeZ()));
		}
		return profile;
	};
	const std::vector<double> a = run(false);
	const std::vector<double> b = run(false);
	const std::vector<double> m = run(true);
	ASSERT_EQ(a.size(), m.size());
	for (std::size_t k = 0; k < a.size(); k++) {
		EXPECT_EQ(a[k], b[k]);
		// stack takes float positions, so the mirrored positions differ by rounding
		EXPECT_NEAR(a[k], m[a.size() - 1 - k], 1e-5);
	}
}

TEST(BlendingSimulatorCone, LaterMaterialWrapsTheEarlierOne)
{
	// Two portions at the same place: the second forms a shell around the first, so the reclaimer's face meets only the second portion
	// where it starts and where it ends, and both in between
	Cone simulator(coneParameters(30.0f, 20.0f, 64.0f));
	simulator.stack(15.0f, 10.0f, bs::AveragedParameters(50.0, {0.0}));
	simulator.stack(15.0f, 10.0f, bs::AveragedParameters(50.0, {1.0}));
	const Reclaimed reclaimed = reclaimAll(simulator);
	std::vector<double> quality;
	double sum = 0.0;
	for (std::size_t k = 0; k < reclaimed.volume.size(); k++) {
		if (reclaimed.volume[k] > 1e-6) {
			quality.push_back(reclaimed.values[k][0]);
			sum += reclaimed.volume[k] * reclaimed.values[k][0];
		}
	}
	ASSERT_GT(quality.size(), 4u);
	EXPECT_NEAR(quality.front(), 1.0, 1e-9);
	EXPECT_NEAR(quality.back(), 1.0, 1e-9);
	EXPECT_LT(*std::min_element(quality.begin(), quality.end()), 0.5);
	EXPECT_NEAR(sum, 50.0, 1e-6);
	EXPECT_NEAR(reclaimed.total(), 100.0, 1e-6);
}

TEST(BlendingSimulatorCone, KeepsMaterialOnTheBed)
{
	// More material than a free pile on the bed can hold: it piles up against the bed edges, nothing is lost
	Cone simulator(coneParameters(20.0f, 10.0f, 8.0f));
	for (int k = 0; k < 20; k++) {
		simulator.stack(10.0f, 5.0f, bs::AveragedParameters(100.0, {1.0}));
	}
	EXPECT_NEAR(reclaimAll(simulator).total(), 2000.0, 1e-6);
}

TEST(BlendingSimulatorCone, ReclaimsAtAnyAngleAndOnCircularBeds)
{
	for (float angle : {30.0f, 45.0f, 60.0f, 90.0f}) {
		Cone simulator(coneParameters(59.0f, 20.0f, 4.0f, angle));
		stackChevron(simulator, 1000.0);
		EXPECT_NEAR(reclaimAll(simulator).total(), 1000.0, 1e-6) << angle;
	}
	bs::SimulationParameters circular = coneParameters(60.0f, 60.0f, 4.0f);
	circular.circular = true;
	Cone simulator(circular);
	const double radius = 15.0;
	for (int k = 0; k < 200; k++) {
		const double angle = 2.0 * pi * k / 50.0;
		simulator.stack(float(30.0 + radius * std::cos(angle)), float(30.0 + radius * std::sin(angle)), bs::AveragedParameters(5.0, {1.0}));
	}
	EXPECT_NEAR(reclaimAll(simulator).total(), 1000.0, 1e-6);
}

TEST(BlendingSimulatorCone, MatchesThePrototypeOfTheSimulatorTrial)
{
	// Reclaimed volumes of the chevron above at 1 m cells, as computed by the prototype of the simulator trial of 2026-10-16
	// (which bins directly into 1 m slices; with 1 m cells the slice widths coincide)
	Cone simulator(coneParameters(59.0f, 20.0f, 1.0f));
	stackChevron(simulator, 2500.0);
	const Reclaimed reclaimed = reclaimAll(simulator);
	const double expected[] = {
		0.000000000, 0.000000000, 0.000000000, 0.000000000, 0.000000000, 3.365048060, 20.818332602, 33.518834482, 41.774755494,
		47.745196303, 52.180375873, 55.393953785, 57.915025844, 59.631091572, 60.804347250, 61.803222102, 62.528128713, 63.035349879,
		63.396503190, 63.636727783, 63.794837222, 63.898188846, 63.964387907, 64.006283599, 64.032227034, 64.046394869, 64.050851073,
		64.046157516, 64.031461164, 64.004363620, 63.960618602, 63.893642623, 63.793847063, 63.646415813, 63.430140572, 63.115395204,
		62.662122324, 62.027405710, 61.157769073, 59.893321831, 58.179658425, 56.157640729, 53.718794152, 50.875078601, 47.592187180,
		43.215030248, 39.061333901, 34.192029358, 29.646551669, 23.799634634, 19.259388011, 13.521278767, 9.475648153, 4.399793222,
		1.903228355, 0.000000000, 0.000000000, 0.000000000, 0.000000000, 0.000000000
	};
	ASSERT_EQ(reclaimed.volume.size(), sizeof(expected) / sizeof(expected[0]));
	for (std::size_t k = 0; k < reclaimed.volume.size(); k++) {
		EXPECT_NEAR(reclaimed.volume[k], expected[k], 1e-6) << "slice " << k;
	}
}

TEST(ContinuousDrops, PartialCellsAtTheEndsGetTheirOverlap)
{
	// 1 m cells, one portion of 3 m3 while the stacker moves from 2.5 to 5.5 m at constant speed: half cells at both ends
	const std::vector<bs::ContinuousDrop> drops = bs::continuousDrops({3.0}, {0.0, 1.0}, {0.0, 1.0}, {2.5, 5.5}, {1.0, 1.0}, 1.0, 1.0);
	ASSERT_EQ(drops.size(), 4u);
	const double volumes[] = {0.5, 1.0, 1.0, 0.5};
	const double positions[] = {2.75, 3.5, 4.5, 5.25};
	for (std::size_t k = 0; k < 4; k++) {
		EXPECT_NEAR(drops[k].volume, volumes[k], 1e-12);
		EXPECT_NEAR(drops[k].x, positions[k], 1e-12);
		EXPECT_EQ(drops[k].z, 1.0);
	}
}

TEST(ContinuousDrops, ConservesTheVolumeAndSkipsGaps)
{
	// Out, a stop, and back; the second portion is a gap without flow
	const std::vector<bs::ContinuousDrop> drops =
		bs::continuousDrops({30.0, 0.0, 50.0, 20.0}, {0.0, 1.0, 1.5, 3.0, 4.0}, {0.0, 1.3, 2.0, 4.0}, {3.3, 17.8, 17.8, 1.2}, {5.0, 5.0, 5.0, 5.0}, 0.7, 0.7);
	double total = 0.0;
	for (const bs::ContinuousDrop& d : drops) {
		total += d.volume;
		EXPECT_NE(d.portion, 1);
	}
	EXPECT_NEAR(total, 100.0, 1e-9);
}
