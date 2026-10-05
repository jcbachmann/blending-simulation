#ifndef BLENDINGSIMULATOR_RECLAIMSLICES_H
#define BLENDINGSIMULATOR_RECLAIMSLICES_H

#include <algorithm>
#include <cmath>
#include <vector>

namespace blendingsimulator
{
// Material collected in slices of equal width along the reclaim direction, reclaimed by a reclaimer moving from position 0 upwards.
// A reclaimer position inside a slice takes the corresponding share of its material.
template<typename Parameters>
class ReclaimSlices
{
	public:
		void resize(unsigned int count, float sliceWidth)
		{
			slices.resize(count);
			width = sliceWidth;
		}

		unsigned int size() const
		{
			return static_cast<unsigned int>(slices.size());
		}

		void clear()
		{
			for (Parameters& slice : slices) {
				slice.clear();
			}
			reclaimerPos = 0.0f;
		}

		void push(int index, const Parameters& parameters)
		{
			slices[index].push(parameters);
		}

		// Spreads a particle over the slices the reclaimer cuts it in. A box of horizontal extent `length` and vertical extent `height` is
		// cut by faces at the reclaim angle, so its reclaim positions are the sum of two uniform distributions of widths `length` and
		// `height / tan(reclaim angle)` (passed as `depth`): a trapezoid around `center`, the reclaim position of the box center. Material
		// beyond the ends goes into the first or last slice, or around the circle if `wrap` is set.
		void push(double center, double length, double depth, const Parameters& parameters, bool wrap = false)
		{
			const int count = static_cast<int>(slices.size());
			const double half = 0.5 * (length + depth);
			const int first = static_cast<int>(std::floor((center - half) / width));
			const int last = static_cast<int>(std::floor((center + half) / width));
			const double volume = parameters.getVolume();

			Parameters rest = parameters;
			double below = 0.0;
			for (int i = first; i <= last; i++) {
				const int index = wrap ? ((i % count) + count) % count : std::max(0, std::min(i, count - 1));
				if (i == last) {
					slices[index].push(rest);
					break;
				}
				const double upTo = trapezoidCdf((i + 1) * double(width) - center, length, depth);
				const double share = std::min((upTo - below) * volume, rest.getVolume());
				below = upTo;
				if (share > 0.0) {
					slices[index].push(rest.pop(share));
				}
			}
		}

		bool finished() const
		{
			return int(reclaimerPos / width + 0.5) >= slices.size();
		}

		Parameters reclaim(float position)
		{
			double oldPos = reclaimerPos / width;
			double newPos = position / width;
			int startPos = static_cast<int>(oldPos);
			int endPos = static_cast<int>(newPos);

			if (startPos < 0) {
				startPos = 0;
			}

			if (endPos > slices.size()) {
				endPos = static_cast<int>(slices.size());
			}

			Parameters p;
			for (int i = startPos; i < endPos; i++) {
				p.push(slices[i]);
				slices[i].clear();
			}

			if (endPos < slices.size()) {
				auto& r = slices[endPos];
				double popVolume = 0.0f;
				if (startPos == endPos) {
					double missingPart = oldPos - double(endPos);
					double div = 1.0f - missingPart;
					if (div > 1e-20) {
						double originalVolume = r.getVolume() / div;
						popVolume = std::min((newPos - oldPos) * originalVolume, r.getVolume());
					} else {
						r.clear();
					}
				} else {
					popVolume = r.getVolume() * (newPos - double(endPos));
				}
				p.push(r.pop(popVolume));
			}

			reclaimerPos = position;
			return p;
		}

	private:
		// Distribution function of the sum of two uniform distributions of widths p and q centered at 0
		static double trapezoidCdf(double t, double p, double q)
		{
			if (p > q) {
				std::swap(p, q);
			}
			if (q <= 0.0) {
				return t >= 0.0 ? 1.0 : 0.0;
			}
			if (p <= 0.0) {
				return std::max(0.0, std::min((t + 0.5 * q) / q, 1.0));
			}
			auto ramp = [](double u) { return u > 0.0 ? 0.5 * u * u : 0.0; };
			const double outer = 0.5 * (p + q);
			const double inner = 0.5 * (q - p);
			const double value = (ramp(t + outer) - ramp(t + inner) - ramp(t - inner) + ramp(t - outer)) / (p * q);
			return std::max(0.0, std::min(value, 1.0));
		}

		std::vector<Parameters> slices;

		// Slice width in m
		float width = 1.0f;

		// Position up to which material has been reclaimed
		float reclaimerPos = 0.0f;
};
}

#endif
