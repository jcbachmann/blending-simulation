#ifndef Parameters_H
#define Parameters_H

// Default parameter definitions

class AveragedParameters
{
	public:
		AveragedParameters()
			: volume(0)
		{
		}

		AveragedParameters(double volume, std::vector<double>&& values)
			: volume(0)
		{
			push(volume, std::move(values));
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

		void push(double otherVolume, std::vector<double>&& otherValues)
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

		AveragedParameters pop(double otherVolume)
		{
			if (volume < otherVolume) {
				throw std::runtime_error("could not pop volume, not enough volume left");
			}

			AveragedParameters result(otherVolume, getValues());

			double newVolume = volume - otherVolume;
			for (int i = 0; i < values.size(); i++) {
				values[i] *= newVolume / volume;
			}
			volume = newVolume;

			return result;
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

#endif
