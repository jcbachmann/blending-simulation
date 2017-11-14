#ifndef Parameters_H
#define Parameters_H

// Default parameter definitions

class AveragedParameters
{
	public:
			: count(0)
		AveragedParameters()
		{
		}

		AveragedParameters(std::vector<double>&& values)
			: count(1)
			, values(values)
		{
		}

		void add(const AveragedParameters& other)
		{
			if (values.empty()) {
				values.resize(other.values.size(), 0.0);
			}

			if (values.size() != other.values.size()) {
				throw std::runtime_error("invalid parameter count");
			}

			count += other.count;
			for (int i = 0; i < values.size(); i++) {
				values[i] += other.values[i];
			}
		}

		double get(unsigned int i)
		{
			if (i < values.size()) {
				return values[i] / double(count);
			} else {
				// Instances which have never been touched have empty values vectors
				return 0.0;
			}
		}

		int getCount(void) const
		{
			return count;
		}

		void clear(void)
		{
			count = 0;
			values.clear();
		}

	private:
		int count;
		std::vector<double> values;
};

class CountedParameters
{
	public:
		CountedParameters(void)
		{
		}

		CountedParameters(unsigned int size, unsigned int incrementIndex)
			: counts(size, 0)
		{
			counts[incrementIndex]++;
		}

		void add(const CountedParameters& other)
		{
			if (counts.empty()) {
				counts.resize(other.counts.size(), 0);
			}

			if (counts.size() != other.counts.size()) {
				throw std::runtime_error("invalid parameter count");
			}

			for (int i = 0; i < counts.size(); i++) {
				counts[i] += other.counts[i];
			}
		}

		int get(unsigned int i) const
		{
			if (i < counts.size()) {
				return counts[i];
			} else {
				// Instances which have never been touched have empty values vectors
				return 0;
			}
		}

		void clear(void)
		{
			counts.clear();
		}

	private:
		std::vector<int> counts;
};

#endif
