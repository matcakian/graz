#include "market.hpp"

#include <cstdint>
#include <stdexcept>
#include <limits>
#include <format>
#include <cassert>

namespace market {

MarketState::MarketState(std::int64_t y_shares, std::int64_t n_shares) :
y_shares_(y_shares), n_shares_(n_shares) {
	if (y_shares < 0 || n_shares < 0)
		throw std::invalid_argument("The number of shares must be non-negative");
}

MarketState::MarketState() : MarketState(0, 0) {}

std::int64_t MarketState::quantity(Outcome outcome) const {
	return (outcome == Outcome::Yes ? y_shares_ : n_shares_);
}

/* adds share units of a certain outcome to the market */
void MarketState::add(Outcome outcome, std::int64_t shares) {
	if (shares < 0)
		throw std::invalid_argument("The number of shares must be positive");

	constexpr std::int64_t MAX_INT64 = std::numeric_limits<std::int64_t>::max();
	if (shares > MAX_INT64 - quantity(outcome))
		throw std::overflow_error("The number of shares overflows std::int64_t range");

	if (outcome == Outcome::Yes) y_shares_ += shares;
	else n_shares_ += shares;
}

LMSR::LMSR(double liquidity) : liquidity_(liquidity) {
	if (std::isnan(liquidity))
		throw std::invalid_argument("Liquidity must not be NaN");

	if (!std::isfinite(liquidity))
		throw std::invalid_argument("Liquidity must be finite");

	if (std::isless(liquidity, 0.0))
		throw std::invalid_argument("Liquidity must be positive");

	if (std::isless(liquidity, MIN_LIQUIDITY))
		throw std::invalid_argument(
			std::format("Liquidity must be at least {:.1f}", MIN_LIQUIDITY));
}

LMSR::LMSR() : LMSR(DEFAULT_LIQUIDITY) {}

double LMSR::liquidity() const {
	return liquidity_;
}

double LMSR::probability(const MarketState& state, Outcome outcome) const {
	const double x = static_cast<double>(state.quantity(outcome)
		- state.quantity(!outcome)) / liquidity_;
	
	if (std::isless(x, 0.0))
		return std::exp(x) / (std::exp(x) + 1.0);
	else
		return 1.0 / (1.0 + std::exp(-x));
}

} // namespace market

