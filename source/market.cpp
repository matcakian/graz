#include "market.hpp"

#include <cstdint>
#include <stdexcept>
#include <limits>
#include <format>
#include <cassert>

namespace market {
MarketState::MarketState(std::int64_t y_units, std::int64_t n_units) :
y_units_(y_units), n_units_(n_units) {
	if (y_units < 0 || n_units < 0)
		throw std::invalid_argument("The number of share units must be non-negative");
}

MarketState::MarketState() : MarketState(0, 0) {}

std::int64_t MarketState::quantity(Outcome outcome) const {
	return (outcome == Outcome::Yes ? y_units_ : n_units_);
}

/* adds share units of a certain outcome to the market */
void MarketState::add(Outcome outcome, std::int64_t s_units) {
	if (s_units < 0)
		throw std::invalid_argument("The number of share units must be positive");

	constexpr std::int64_t max_int64 = std::numeric_limits<std::int64_t>::max();
	if (s_units > max_int64 - quantity(outcome))
		throw std::overflow_error("The number of share units overflows std::int64_t range");

	if (outcome == Outcome::Yes) y_units_ += s_units;
	else n_units_ += s_units;
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
	double q_1 = static_cast<double>(state.quantity(outcome)) / UNITS_PER_SHARE;
	double q_2 = static_cast<double>(state.quantity(!outcome)) / UNITS_PER_SHARE;

	double x = (q_1 - q_2) / liquidity_;

	double result;
	if (std::isless(x, 0.0))
		result = std::exp(x) / (std::exp(x) + 1.0);
	else
		result = 1.0 / (1.0 + std::exp(-x));

	return result;
}

namespace {
struct Bound {
	double lower, upper;

	Bound(double lower, double upper) : lower(lower), upper(upper) {
		assert(std::isgreaterequal(upper, lower));
	}
};
}
}
