#ifndef Parameters_H
#define Parameters_H

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace blendingsimulator
{
class AveragedParameters
{
	public:
		AveragedParameters()
			: volume(0)
		{
		}

		AveragedParameters(double volume, const std::vector<double>& values)
			: volume(0)
		{
			push(volume, values);
		}

		void push(const AveragedParameters& other)
		{
			if (other.empty()) {
				return;
			}

			if (values.empty()) {
				values.resize(other.values.size(), 0.0);
			}

			if (values.size() != other.values.size()) {
				throw std::runtime_error("invalid parameter count");
			}

			volume += other.volume;
			for (int i = 0; i < values.size(); i++) {
				values[i] += other.values[i]; // Values already weighted
			}
		}

		void push(double otherVolume, const std::vector<double>& otherValues)
		{
			if (values.empty()) {
				values.resize(otherValues.size(), 0.0);
			}

			if (values.size() != otherValues.size()) {
				throw std::runtime_error("invalid parameter count");
			}

			volume += otherVolume;
			for (int i = 0; i < values.size(); i++) {
				values[i] += otherVolume * otherValues[i];
			}
		}

		bool contains(double otherVolume)
		{
			return volume >= otherVolume;
		}

		// Adds `otherVolume` of material with the composition of `other`, whatever volume `other` holds itself; does not allocate once the
		// values are sized
		void push(const AveragedParameters& other, double otherVolume)
		{
			if (otherVolume <= 0.0 || other.volume < 1e-100) {
				return;
			}

			if (values.empty()) {
				values.resize(other.values.size(), 0.0);
			}

			if (values.size() != other.values.size()) {
				throw std::runtime_error("invalid parameter count");
			}

			const double factor = otherVolume / other.volume;
			volume += otherVolume;
			for (std::size_t i = 0; i < values.size(); i++) {
				values[i] += factor * other.values[i];
			}
		}

		AveragedParameters pop(double otherVolume)
		{
			AveragedParameters result;
			result.push(*this, otherVolume);
			remove(otherVolume);
			return result;
		}

		// Takes away `otherVolume` without changing the composition
		void remove(double otherVolume)
		{
			if (volume < otherVolume) {
				throw std::runtime_error("could not pop volume, not enough volume left");
			}

			const double newVolume = volume - otherVolume;
			const double factor = volume > 0.0 ? newVolume / volume : 0.0;
			for (double& value : values) {
				value *= factor;
			}
			volume = newVolume;
		}

		double getVolume() const
		{
			return volume;
		}

		double getValue(unsigned int i) const
		{
			if (i < values.size() && volume > 1e-100) {
				return values[i] / volume;
			} else {
				// Instances which have never been touched have empty values vectors
				return 0.0;
			}
		}

		std::vector<double> getValues() const
		{
			std::vector<double> result(values.begin(), values.end());
			if (volume > 1e-100) {
				for (double& v : result) {
					v /= volume;
				}
			}
			return result;
		}

		void clear()
		{
			volume = 0.0;
			values.clear();
		}

		bool empty() const
		{
			return values.empty() && volume < 1e-100;
		}

	private:
		double volume;
		std::vector<double> values;
};
}

#endif
