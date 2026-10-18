#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "BlendingSimulator/BlendingSimulatorFormula.h"
#include "BlendingSimulator/ContinuousDrops.h"
#include "BlendingSimulator/ParticleParameters.h"
#ifdef CONE_SIMULATOR_AVAILABLE
#include "BlendingSimulator/BlendingSimulatorCone.h"
#endif

namespace bs = blendingsimulator;
using Formula = bs::BlendingSimulatorFormula<bs::AveragedParameters>;

const double pi = std::atan(1.0) * 4.0;

bs::SimulationParameters formulaParameters(float sizeX, float sizeZ, float particlesPerCubicMeter, float reclaimAngle = 45.0f)
{
	bs::SimulationParameters simulationParameters;
	simulationParameters.heapWorldSizeX = sizeX;
	simulationParameters.heapWorldSizeZ = sizeZ;
	simulationParameters.reclaimAngle = reclaimAngle;
	simulationParameters.particlesPerCubicMeter = particlesPerCubicMeter;
	return simulationParameters;
}

// Reclaims everything in steps of 1 m like the Python bindings: volumes and the values of the parameters per slice
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
void stackChevron(bs::BlendingSimulator<bs::AveragedParameters>& simulator, double totalVolume, float z = 10.0f)
{
	const int passes = 10;
	const int steps = 39;
	const double portion = totalVolume / (passes * steps);
	for (int pass = 0; pass < passes; pass++) {
		for (int s = 0; s < steps; s++) {
			const double x = pass % 2 == 0 ? 10.5 + s : 48.5 - s;
			const double quality = double(pass * steps + s) / (passes * steps);
			simulator.stack(float(x), z, bs::AveragedParameters(portion, {quality, 1.0 - quality}));
		}
	}
}

// 50 portions of equal volume flowing in equal times while the stacker follows a path of 20 knots in equal times along the line z, dropped
// once per cell it crosses (ContinuousDrops.h), the portions' qualities between 5 and 10
void stackPath(bs::BlendingSimulator<bs::AveragedParameters>& simulator, double totalVolume, double z, double cellX)
{
	const int portions = 50;
	const int knots = 20;
	const double path[knots] = {
		12.0, 47.5, 30.2, 10.0, 49.0, 22.7, 38.1, 15.4, 44.9, 27.3, 33.3, 11.8, 46.2, 19.5, 41.0, 25.6, 36.8, 13.9, 48.1, 29.0
	};
	const std::vector<double> pathX(path, path + knots);
	std::vector<double> volumes(portions, totalVolume / portions), portionTimes(portions + 1), pathTimes(knots), pathZ(knots, z);
	for (int i = 0; i <= portions; i++) {
		portionTimes[i] = double(i) / portions;
	}
	for (int i = 0; i < knots; i++) {
		pathTimes[i] = double(i) / (knots - 1);
	}
	for (const bs::ContinuousDrop& d : bs::continuousDrops(volumes, portionTimes, pathTimes, pathX, pathZ, cellX, 20.0)) {
		const double quality = 5.0 + 2.5 * (1.0 + std::sin(1.7 * d.portion));
		simulator.stack(float(d.x), float(d.z), bs::AveragedParameters(d.volume, {quality}));
	}
}

void expectProfiles(const Reclaimed& reclaimed, const std::vector<double>& volume, const std::vector<double>& quality, const char* name)
{
	ASSERT_EQ(reclaimed.volume.size(), volume.size()) << name;
	for (std::size_t k = 0; k < volume.size(); k++) {
		// The prototype holds each drop's volume to 1e-7 of it, the simulation exactly
		EXPECT_NEAR(reclaimed.volume[k], volume[k], 1e-7 * std::max(1.0, volume[k])) << name << ", slice " << k;
		if (volume[k] > 1e-3) {
			EXPECT_NEAR(reclaimed.values[k][0], quality[k], 1e-8 * std::max(1.0, quality[k])) << name << ", slice " << k;
		}
	}
}

TEST(BlendingSimulatorFormula, MatchesThePrototype)
{
	// Reclaimed volumes and qualities computed by the prototype of the formula (the envelope model of 2026-10-17) for the same drops, with
	// the drop positions rounded to float as stack takes them
	{
		// chevron, 1 m cells
		Formula simulator(formulaParameters(59.0f, 20.0f, 1.0f));
		stackChevron(simulator, 2500.0);
		const std::vector<double> volume = {
			0.0, 0.0, 0.0, 0.0, 0.0, 3.454982275, 20.782791716, 33.907068520, 42.201489520, 47.574791471, 51.897591894, 54.874720330,
			57.582328940, 59.442953294, 60.701645181, 61.649123914, 62.410327407, 62.921519434, 63.303520934, 63.591402279, 63.766460092,
			63.877055316, 63.949969364, 63.997623661, 64.027689389, 64.044964790, 64.052180120, 63.969430415, 63.995421997, 64.017257865,
			63.980415280, 63.923098559, 63.836554797, 63.708026176, 63.519296031, 63.243468538, 62.800079678, 62.137924074, 61.262039654,
			60.094905622, 58.403891465, 56.389310603, 53.900620870, 50.936187432, 47.631424103, 43.279053069, 39.236300713, 34.032489919,
			29.440727376, 23.834709181, 19.118247728, 13.586716538, 9.379607437, 4.463492534, 1.867119689, 0.0, 0.0, 0.0, 0.0, 0.0
		};
		const std::vector<double> quality = {
			0.0, 0.0, 0.0, 0.0, 0.0, 0.950056536, 0.834790513, 0.692200725, 0.592970866, 0.543220909, 0.516460317, 0.503192417,
			0.496601399, 0.495169506, 0.494088829, 0.493511436, 0.493872169, 0.494238134, 0.494803518, 0.495409440, 0.495758295,
			0.495974554, 0.496123586, 0.496224129, 0.496286671, 0.496317293, 0.496318296, 0.496731679, 0.496465615, 0.496113346,
			0.495936466, 0.495671301, 0.495282138, 0.494718984, 0.493913873, 0.492774594, 0.490869870, 0.488034982, 0.484552089,
			0.480469110, 0.474362495, 0.467879884, 0.460884772, 0.454220195, 0.451256686, 0.441439623, 0.442898677, 0.442924063,
			0.458615726, 0.465433221, 0.515530196, 0.549642618, 0.654985120, 0.713895806, 0.863732672, 0.414786173, 0.414284779,
			0.414166719, 0.414051443, 0.497666440
		};
		expectProfiles(reclaimAll(simulator), volume, quality, "chevron, 1 m cells");
	}
	{
		// path, 1 m cells
		Formula simulator(formulaParameters(59.0f, 20.0f, 1.0f));
		stackPath(simulator, 2500.0, 10.0, 1.0);
		const std::vector<double> volume = {
			0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2.339937392, 11.461728982, 21.101808853, 28.250771770, 34.318549038, 38.505252900,
			42.705205562, 47.317715059, 51.827876327, 56.752971255, 62.051755024, 67.815868373, 74.766248764, 80.428499412, 86.919792516,
			91.044959263, 95.118510389, 96.999218262, 98.855950311, 100.944983464, 99.096775839, 98.189928456, 96.468242892, 93.566573136,
			90.443081683, 86.591677627, 82.307580205, 77.993031798, 73.203567582, 68.069879387, 63.336219964, 58.390899931, 53.123621134,
			48.149734527, 43.228368290, 38.082128027, 33.257401252, 28.509039701, 24.011978304, 19.142838443, 14.800574763, 10.220323612,
			6.756351656, 2.765243376, 0.767356565, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
		};
		const std::vector<double> quality = {
			0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 6.003060642, 6.107626378, 6.226420731, 6.351339828, 6.489082719, 6.547821529,
			6.643673774, 6.855578302, 7.104740596, 7.297266548, 7.408096724, 7.442231317, 7.471007096, 7.636874349, 7.843326835,
			8.087353259, 8.265479178, 8.346361239, 8.377786104, 8.310783957, 8.204601409, 8.095433876, 7.996055448, 7.920745643,
			7.868252219, 7.830137826, 7.800747463, 7.794173106, 7.702319560, 7.455313177, 7.248621200, 7.062884255, 6.880699828,
			6.716813793, 6.595524915, 6.485293392, 6.538067980, 6.454897590, 6.305773698, 6.128495116, 5.917996069, 5.606951617,
			5.143944494, 5.072993378, 5.055242463, 7.530177672, 7.530723803, 7.531170090, 7.531527332, 7.531852173, 7.532162321,
			7.609093272
		};
		expectProfiles(reclaimAll(simulator), volume, quality, "path, 1 m cells");
	}
	{
		// path, 0.5 m cells
		Formula simulator(formulaParameters(59.0f, 20.0f, 8.0f));
		stackPath(simulator, 2500.0, 10.0, 0.5);
		const std::vector<double> volume = {
			0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 2.519826535, 11.864129716, 21.258562099, 28.451964354, 34.271027722, 38.329210840,
			42.345071965, 47.182817384, 51.056339114, 56.086744450, 61.619535908, 68.008696428, 73.979848624, 80.178580769, 86.101650920,
			89.628105995, 93.626747233, 97.386623750, 99.528084500, 100.865890774, 100.587820435, 98.797021065, 96.593838558, 94.162434801,
			90.769061199, 86.829433427, 82.997071347, 78.268284115, 73.650773723, 68.217246807, 63.605221625, 58.560025821, 53.378316911,
			48.234533422, 43.061866187, 38.337187414, 33.370815943, 28.612749913, 23.844189110, 19.195988757, 14.644709711, 10.182693019,
			6.340138064, 2.946604638, 0.522535406, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
		};
		const std::vector<double> quality = {
			0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 5.994974934, 6.110755929, 6.210826081, 6.306257001, 6.445513897, 6.536873184,
			6.631163306, 6.853178773, 7.099688970, 7.299630170, 7.393100041, 7.401830640, 7.467166658, 7.659160605, 7.858101888,
			8.140464036, 8.315027376, 8.408758220, 8.415742852, 8.342420349, 8.220387240, 8.081162132, 7.963460656, 7.877368517,
			7.818167053, 7.777294815, 7.750090934, 7.749273780, 7.658106253, 7.418208866, 7.240734448, 7.086925059, 6.920131500,
			6.738392028, 6.609352258, 6.552917556, 6.563801465, 6.484584150, 6.334302924, 6.158892548, 5.950677314, 5.596996510,
			5.183454051, 5.069864842, 5.056672413, 7.464739510, 7.466323660, 7.465381177, 7.464466292, 7.463612671, 7.462801599,
			7.558105117
		};
		expectProfiles(reclaimAll(simulator), volume, quality, "path, 0.5 m cells");
	}
	{
		// path, angle of repose 35 degrees
		Formula simulator(formulaParameters(59.0f, 20.0f, 1.0f), 35.0);
		stackPath(simulator, 2500.0, 10.0, 1.0);
		const std::vector<double> volume = {
			0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.095804530, 3.227103173, 10.705707647, 19.844649799, 27.553162164, 32.852656682,
			37.184055859, 40.787594722, 45.103730211, 49.670560808, 54.188202479, 59.177130088, 64.167588221, 70.607041989, 75.742072849,
			82.650219001, 88.502588524, 92.269990689, 95.060442258, 97.231834913, 98.474762150, 97.132002561, 95.651955644, 95.011859470,
			93.122703863, 89.721047931, 86.771878779, 83.771643660, 79.461935783, 74.721173067, 71.051708453, 65.849925041, 61.698369080,
			56.615516236, 51.710170578, 46.582171444, 41.387709700, 36.482372931, 31.697391638, 26.576904949, 22.117505307, 17.486784744,
			12.925455094, 9.472722729, 5.043765839, 2.556842125, 0.281607308, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
		};
		const std::vector<double> quality = {
			0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 5.956268033, 6.008099831, 6.107619363, 6.164546007, 6.294343353, 6.440330673, 6.487547129,
			6.539867657, 6.721694170, 7.109030189, 7.373016939, 7.382597062, 7.382452586, 7.443385055, 7.537491230, 7.758861189,
			8.074929488, 8.321287922, 8.465435354, 8.488011732, 8.401384118, 8.273848142, 8.117696329, 7.990271863, 7.908831055,
			7.844512872, 7.806399913, 7.793857176, 7.769377522, 7.695622645, 7.452068811, 7.301204549, 7.154929937, 7.004351941,
			6.842174333, 6.696565939, 6.585862852, 6.641582759, 6.576695658, 6.483132222, 6.288515646, 6.093530732, 5.854600955,
			5.452098850, 5.167118498, 5.061196067, 5.055242463, 7.329191285, 7.331265323, 7.333301169, 7.335205533, 7.336964615,
			7.605982841
		};
		expectProfiles(reclaimAll(simulator), volume, quality, "path, angle of repose 35 degrees");
	}
	{
		// path, reclaim angle 60 degrees
		Formula simulator(formulaParameters(59.0f, 20.0f, 1.0f, 60.0f));
		stackPath(simulator, 2500.0, 10.0, 1.0);
		const std::vector<double> volume = {
			0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.621451718, 5.099784470, 12.548013240, 19.981664790, 28.175899390, 33.258686773,
			37.244020377, 40.672639040, 44.633549783, 48.951106910, 52.986182619, 57.643086059, 62.314920610, 68.200049045, 74.283783470,
			80.637429713, 87.124960031, 92.817981333, 97.811962423, 100.564975395, 102.464508044, 103.224219351, 101.720627776,
			100.828032939, 98.208723660, 95.466059516, 91.524153882, 87.143011479, 82.244855903, 77.215173958, 71.327688913, 66.071085951,
			60.339752212, 55.094355402, 49.394033257, 44.277097221, 38.749700981, 33.662755036, 28.676765326, 23.171520453, 17.977130208,
			13.094530526, 7.791579177, 3.931287433, 0.829225273, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
		};
		const std::vector<double> quality = {
			0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 5.954657219, 5.985901687, 6.110432084, 6.273855423, 6.484770028, 6.524991318,
			6.443895263, 6.472966707, 6.600328594, 7.029456203, 7.448712824, 7.448681812, 7.375683692, 7.465376852, 7.588705369,
			7.745463819, 8.018188171, 8.276467716, 8.465154124, 8.475207846, 8.385943704, 8.204112464, 8.059695500, 7.945698531,
			7.917720995, 7.895594877, 7.890219630, 7.905864915, 7.864557149, 7.635193635, 7.311007709, 7.071800534, 6.977535074,
			6.833337770, 6.741414383, 6.630932417, 6.530116590, 6.560451514, 6.346773497, 6.226359499, 5.936204258, 5.652380471,
			5.143265953, 5.071130877, 5.055242463, 7.498528031, 7.497804860, 7.497248103, 7.496848391, 7.496504761, 7.496223405,
			7.193756824
		};
		expectProfiles(reclaimAll(simulator), volume, quality, "path, reclaim angle 60 degrees");
	}
	{
		// path, 6,000 m3 against the walls, stacker line at z = 7 m
		Formula simulator(formulaParameters(59.0f, 20.0f, 1.0f));
		stackPath(simulator, 6000.0, 7.0, 1.0);
		const std::vector<double> volume = {
			0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 6.199943235, 38.093308367, 61.179526216, 77.807931269, 94.098469103, 109.021090324,
			122.106688657, 138.740927061, 152.358767577, 164.100655458, 176.945113992, 186.778485687, 196.402516206, 201.731495627,
			203.483284830, 206.816517459, 208.482534152, 208.291871705, 205.754817898, 203.984818812, 200.893413831, 196.277917506,
			193.192090813, 185.530247602, 180.588856761, 174.312829185, 168.501746063, 161.425488367, 153.156048457, 145.927011215,
			139.097970067, 131.149049163, 123.199279131, 115.265291886, 107.657952549, 99.884189491, 91.534730661, 82.684150994,
			74.483273832, 65.592283906, 56.869475216, 48.566041807, 41.001578554, 33.187052855, 26.188221857, 19.004598994, 12.677535515,
			6.833262443, 2.939702833, 0.0, 0.0, 0.0, 0.0, 0.0
		};
		const std::vector<double> quality = {
			0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 6.183448141, 6.130023896, 6.185343450, 6.344066881, 6.489973454, 6.731070824, 6.992504660,
			7.160347956, 7.299368464, 7.408246772, 7.522022588, 7.630096079, 7.736265869, 7.830002652, 7.953324911, 8.149028929,
			8.314756156, 8.351041680, 8.303622212, 8.230838847, 8.160890010, 8.087146137, 7.995196181, 7.913491952, 7.818892858,
			7.743737651, 7.705127315, 7.687712501, 7.643227033, 7.522709914, 7.399689875, 7.306930610, 7.212577178, 7.122852982,
			6.949871600, 6.825520033, 6.715440056, 6.737608548, 6.695412754, 6.669025565, 6.604893314, 6.544811921, 6.407643084,
			6.320197909, 6.127474432, 5.912361183, 5.576533621, 5.360257686, 5.055242463, 7.126471842, 7.120942710, 7.120487884,
			7.120044674, 7.485746717
		};
		expectProfiles(reclaimAll(simulator), volume, quality, "path, 6,000 m3 against the walls, stacker line at z = 7 m");
	}
}

TEST(BlendingSimulatorFormula, ConservesVolumeAndParameters)
{
	for (float ppm3 : {1.0f, 8.0f, 64.0f}) {
		Formula simulator(formulaParameters(59.0f, 20.0f, ppm3));
		stackChevron(simulator, 2500.0);
		const Reclaimed reclaimed = reclaimAll(simulator);
		EXPECT_NEAR(reclaimed.total(), 2500.0, 1e-9) << "ppm3 " << ppm3;
		// Both parameters are carried along: their volume-weighted sum stays 1 in every slice
		for (std::size_t k = 0; k < reclaimed.volume.size(); k++) {
			if (reclaimed.volume[k] > 1e-9) {
				EXPECT_NEAR(reclaimed.values[k][0] + reclaimed.values[k][1], 1.0, 1e-9);
			}
		}
	}
}

TEST(BlendingSimulatorFormula, SingleConeHasTheAnalyticShape)
{
	// On the empty bed a portion of volume V forms a cone of height cbrt(3 V tan(angle)^2 / pi); along the bed the volume is integrated over
	// the cells, so the apex converges with the cell size. Across the bed the surface is exact: at the cell of the apex it falls at the angle
	// of repose
	const double volume = 100.0;
	for (double angle : {30.0, 35.0, 45.0}) {
		const double k = std::tan(angle * pi / 180.0);
		const double expected = std::cbrt(3.0 * volume * k * k / pi);
		Formula simulator(formulaParameters(40.0f, 40.0f, 512.0f), angle);
		simulator.stack(20.0625f, 20.0f, bs::AveragedParameters(volume, {1.0}));
		EXPECT_NEAR(simulator.getLastApex(), expected, 1e-3 * expected) << angle;
		EXPECT_NEAR(simulator.getHeight(20.0625, 20.0), simulator.getLastApex(), 1e-12) << angle;
		for (double d : {0.5, 1.0, 2.0, 3.0}) {
			EXPECT_NEAR(simulator.getHeight(20.0625, 20.0 + d), simulator.getLastApex() - k * d, 1e-9) << angle << ", " << d;
			EXPECT_NEAR(simulator.getHeight(20.0625, 20.0 - d), simulator.getLastApex() - k * d, 1e-9) << angle << ", " << d;
		}
		EXPECT_NEAR(reclaimAll(simulator).total(), volume, 1e-9);
	}
}

TEST(BlendingSimulatorFormula, IsDeterministicAndHasAHeightMap)
{
	auto run = []() {
		Formula simulator(formulaParameters(59.0f, 20.0f, 1.0f));
		stackPath(simulator, 2500.0, 10.0, 1.0);
		const auto size = simulator.getHeapMapSize();
		EXPECT_EQ(size.first, 59u);
		EXPECT_EQ(size.second, 20u);
		const float* heights = simulator.getHeapMap();
		// the highest point lies on the stacker line, the pile falls towards the walls
		const float center = heights[10 * size.first + 29];
		EXPECT_GT(center, 5.0f);
		EXPECT_LT(heights[0 * size.first + 29], center);
		return reclaimAll(simulator).volume;
	};
	const std::vector<double> a = run();
	const std::vector<double> b = run();
	ASSERT_EQ(a.size(), b.size());
	for (std::size_t k = 0; k < a.size(); k++) {
		EXPECT_EQ(a[k], b[k]);
	}
}

TEST(BlendingSimulatorFormula, LaterMaterialWrapsTheEarlierOne)
{
	// Two portions at the same place: the second forms a shell around the first, so the reclaimer's face meets only the second portion
	// where it starts and where it ends, and both in between
	Formula simulator(formulaParameters(30.0f, 20.0f, 64.0f));
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
	EXPECT_NEAR(sum, 50.0, 1e-9);
	EXPECT_NEAR(reclaimed.total(), 100.0, 1e-9);
}

TEST(BlendingSimulatorFormula, KeepsMaterialOnTheBed)
{
	// More material than a free pile on the bed can hold: it piles up against the walls and the ends of the bed, nothing is lost
	Formula simulator(formulaParameters(20.0f, 10.0f, 8.0f));
	for (int k = 0; k < 20; k++) {
		simulator.stack(10.0f, 5.0f, bs::AveragedParameters(100.0, {1.0}));
	}
	EXPECT_NEAR(reclaimAll(simulator).total(), 2000.0, 1e-9);
}

TEST(BlendingSimulatorFormula, ReclaimsAtAnyAngleAndOffCenter)
{
	for (float angle : {0.0f, 30.0f, 45.0f, 60.0f, 90.0f, 120.0f}) {
		Formula simulator(formulaParameters(59.0f, 20.0f, 4.0f, angle));
		stackChevron(simulator, 1000.0);
		EXPECT_NEAR(reclaimAll(simulator).total(), 1000.0, 1e-9) << angle;
	}
	// A stacker line near one wall: the pile leans against it
	Formula simulator(formulaParameters(59.0f, 20.0f, 1.0f));
	stackChevron(simulator, 4000.0, 3.0f);
	EXPECT_NEAR(reclaimAll(simulator).total(), 4000.0, 1e-9);
}

TEST(BlendingSimulatorFormula, RejectsLateralMotionAndCircularBeds)
{
	Formula simulator(formulaParameters(59.0f, 20.0f, 1.0f));
	simulator.stack(20.0f, 10.0f, bs::AveragedParameters(10.0, {1.0}));
	simulator.stack(30.0f, 10.0f, bs::AveragedParameters(0.0, {1.0}));
	EXPECT_THROW(simulator.stack(30.0f, 12.0f, bs::AveragedParameters(10.0, {1.0})), std::invalid_argument);
	// a new stockpile may use another line
	simulator.clear();
	simulator.stack(30.0f, 12.0f, bs::AveragedParameters(10.0, {1.0}));
	EXPECT_NEAR(reclaimAll(simulator).total(), 10.0, 1e-9);

	bs::SimulationParameters circular = formulaParameters(60.0f, 60.0f, 1.0f);
	circular.circular = true;
	EXPECT_THROW(Formula{circular}, std::invalid_argument);
}

#ifdef CONE_SIMULATOR_AVAILABLE
TEST(BlendingSimulatorFormula, AgreesWithTheConeSimulation)
{
	// The formula is the cone simulation reduced to the stacker line. It reclaims each piece of a shell in at most two height bands, which is
	// coarser than the cone simulation's columns: the slice volumes differ by about 1 % of their mean (rms), F1 and the spread of the slice
	// volumes by less than 0.3 %
	for (float ppm3 : {1.0f, 64.0f}) {
		bs::BlendingSimulatorCone<bs::AveragedParameters> cone(formulaParameters(59.0f, 20.0f, ppm3));
		Formula formula(formulaParameters(59.0f, 20.0f, ppm3));
		stackPath(cone, 2500.0, 10.0, formula.getCellSizeX());
		stackPath(formula, 2500.0, 10.0, formula.getCellSizeX());
		const Reclaimed a = reclaimAll(cone);
		const Reclaimed b = reclaimAll(formula);
		ASSERT_EQ(a.volume.size(), b.volume.size());
		auto f1 = [](const Reclaimed& r) {
			double mean = 0.0;
			for (std::size_t k = 0; k < r.volume.size(); k++) {
				mean += r.volume[k] * (r.volume[k] > 0.0 ? r.values[k][0] : 0.0);
			}
			mean /= r.total();
			double variance = 0.0;
			for (std::size_t k = 0; k < r.volume.size(); k++) {
				if (r.volume[k] > 0.0) {
					variance += r.volume[k] * (r.values[k][0] - mean) * (r.values[k][0] - mean);
				}
			}
			return std::sqrt(variance / r.total());
		};
		auto spread = [](const Reclaimed& r) {
			const double mean = r.total() / double(r.volume.size());
			double sum = 0.0;
			for (double v : r.volume) {
				sum += (v - mean) * (v - mean);
			}
			return std::sqrt(sum / double(r.volume.size()));
		};
		double squares = 0.0;
		for (std::size_t k = 0; k < a.volume.size(); k++) {
			squares += (a.volume[k] - b.volume[k]) * (a.volume[k] - b.volume[k]);
		}
		const double meanVolume = a.total() / double(a.volume.size());
		EXPECT_LT(std::sqrt(squares / double(a.volume.size())), 0.02 * meanVolume) << "ppm3 " << ppm3;
		EXPECT_NEAR(f1(b), f1(a), 0.005 * f1(a)) << "ppm3 " << ppm3;
		EXPECT_NEAR(spread(b), spread(a), 0.005 * spread(a)) << "ppm3 " << ppm3;
	}
}
#endif
