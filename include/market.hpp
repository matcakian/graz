#pragma once

#include <cstdint>


/* the payout of one share is one credit */

namespace market {

enum class Outcome {Yes, No};
constexpr Outcome operator!(Outcome outcome) {
	return (outcome == Outcome::Yes ? Outcome::No : Outcome::Yes);
}

class MarketState {
private:
	/* the amount of shares of each outcome in the market */
	std::int64_t y_shares_;
	std::int64_t n_shares_;

public:
	explicit MarketState(std::int64_t y_shares, std::int64_t n_shares);
	MarketState();

	/* returns the amount of shares of a certain outcome in the market */
	[[nodiscard]] std::int64_t quantity(Outcome outcome) const;

	/* adds shares of a certain outcome to the market */
	void add(Outcome outcome, std::int64_t shares);
};

class LMSR {
private:
	static constexpr double DEFAULT_LIQUIDITY = 100.0;
	static constexpr double MIN_LIQUIDITY = 1.0;

	double liquidity_;

	[[nodiscard]] double cost_difference(const MarketState& state,
		Outcome outcome, std::int64_t shares) const;

public:
	explicit LMSR(double liquidity);
	LMSR();

	[[nodiscard]] double liquidity() const;
	[[nodiscard]] double probability(const MarketState& state, Outcome outcome) const;

	/* returns the price of buying a certain amount of shares of a certain outcome, 
	given the market state */
	[[nodiscard]] std::int64_t quote_buy(const MarketState& state, 
		Outcome outcome, std::int64_t shares) const;
};

}

