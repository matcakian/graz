#pragma once

#include <cstdint>

namespace market {
/* The payout of one share is one credit */
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

	// the amount of shares of a certain outcome in the market
	[[nodiscard]] double quantity(Outcome outcome) const;

	[[nodiscard]] bool add(Outcome outcome, std::int64_t share_units);
};

class LMSR {
private:
	static constexpr double DEFAULT_LIQUIDITY = 100.0;
	static constexpr double MIN_LIQUIDITY = 1.0;

	double liquidity_;

public:
	explicit LMSR(double liquidity);
	LMSR();

	[[nodiscard]] double liquidity() const;
	[[nodiscard]] double probability(const MarketState& state, Outcome outcome) const;

	[[nodiscard]] std::int64_t quote_buy(const MarketState& state, 
		Outcome outcome, std::int64_t share_units) const;
};
}

