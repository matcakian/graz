#pragma once

#include <cstdint>

namespace lmsr {
	enum class Outcome {Yes, No};

	class LMSR {
	private:
		static constexpr double DEFAULT_LIQUIDITY = 100.0;

		std::int64_t yes_share_units;
		std::int64_t no_share_units;

		double liquidity;

	public:
		static constexpr int64_t UNITS_PER_SHARE = 1000;
		static constexpr int64_t UNITS_PER_CREDIT = 1000;
		static constexpr int64_t CREDITS_PER_SHARE = 1;

		explicit LMSR(double liquidity);
		LMSR();

		double price(Outcome outcome) const;
		double cost(std::int64_t y_units, std::int64_t n_units) const;
		double cost() const;
		std::int64_t quote_buy(Outcome outcome, std::int64_t share_units) const;
	};
}