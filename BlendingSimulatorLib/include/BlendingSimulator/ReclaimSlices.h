#ifndef BLENDINGSIMULATOR_RECLAIMSLICES_H
#define BLENDINGSIMULATOR_RECLAIMSLICES_H

#include <algorithm>
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
		std::vector<Parameters> slices;

		// Slice width in m
		float width = 1.0f;

		// Position up to which material has been reclaimed
		float reclaimerPos = 0.0f;
};
}

#endif
