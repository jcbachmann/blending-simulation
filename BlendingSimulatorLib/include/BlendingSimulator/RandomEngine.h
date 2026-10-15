#ifndef BLENDINGSIMULATOR_RANDOMENGINE_H
#define BLENDINGSIMULATOR_RANDOMENGINE_H

#include <cstdint>
#include <limits>

namespace blendingsimulator
{
// xoshiro256++ by David Blackman and Sebastiano Vigna (https://prng.di.unimi.it/), seeded through splitmix64. A small, fast generator of
// high statistical quality, usable with the distributions of <random>. The fast simulation draws one number per fall step, where
// std::mt19937 took about a fifth of the run time.
class RandomEngine
{
	public:
		using result_type = std::uint64_t;

		explicit RandomEngine(std::uint64_t seed)
		{
			for (std::uint64_t& word : state) {
				seed += 0x9e3779b97f4a7c15ull;
				std::uint64_t z = seed;
				z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
				z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
				word = z ^ (z >> 31);
			}
		}

		static constexpr result_type min()
		{
			return 0;
		}

		static constexpr result_type max()
		{
			return std::numeric_limits<result_type>::max();
		}

		result_type operator()()
		{
			const std::uint64_t result = rotl(state[0] + state[3], 23) + state[0];
			const std::uint64_t t = state[1] << 17;
			state[2] ^= state[0];
			state[3] ^= state[1];
			state[1] ^= state[2];
			state[0] ^= state[3];
			state[2] ^= t;
			state[3] = rotl(state[3], 45);
			return result;
		}

	private:
		std::uint64_t state[4];

		static std::uint64_t rotl(std::uint64_t x, int k)
		{
			return (x << k) | (x >> (64 - k));
		}
};
}

#endif
