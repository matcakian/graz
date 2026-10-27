#pragma once

#include <cstdint>

namespace lmsr {
constexpr std::int64_t UNITS_PER_SHARE = 1000;
constexpr std::int64_t UNITS_PER_CREDIT = 1000;

enum class Outcome {Yes, No};

class MarketState {
private:
	/* the amount of share units of each outcome in the market */
	std::int64_t y_units_;
	std::int64_t n_units_;

public:
	explicit MarketState(std::int64_t y_units, std::int64_t n_units);
	MarketState();
}

class LMSR {
private:
	MarketState state;
	double liquidity_;

public:
	explicit LMSR(double liquidity);
	LMSR();

	[[nodiscard]] double liquidity() const;

	[[nodiscard]] double probability(Outcome outcome) const;
	[[nodiscard]] std::int64_t quote_buy(std::int64_t share_units, Outcome outcome) const;
}
}