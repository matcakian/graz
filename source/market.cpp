#include "market.hpp"

#include <cstdint>
#include <stdexcept>
#include <limits>
#include <format>
#include <string>

namespace market {
	MarketState::MarketState(std::int64_t y_units, std::int64_t n_units) :
	y_units_(y_units), n_units_(n_units) {
		if (y_units < 0 || n_units < 0)
			throw std::invalid_argument("Share units must be positive");
	}

	MarketState::MarketState() : MarketState(0, 0) {}

	double MarketState::quantity(Outcome outcome) const {
		// referenced units
		const int64_t& r_units = (outcome == Outcome::Yes ? y_units_ : n_units_);

		return static_cast<double>(r_units) / UNITS_PER_SHARE;
	}

	bool MarketState::add(Outcome outcome, std::int64_t share_units) {
		// referenced units
		int64_t& r_units = (outcome == Outcome::Yes ? y_units_ : n_units_);

		constexpr std::int64_t MAX_INT64 = std::numeric_limits<std::int64_t>::max();
		if (share_units > MAX_INT64 - r_units)
			return false;

		r_units += share_units;
		return true;
	}

	LMSR::LMSR(double liquidity) : liquidity_(liquidity) {
		if (std::isless(liquidity, 0.0))
			throw std::invalid_argument("Liquidity must be positive");

		if (std::isless(liquidity, MIN_LIQUIDITY))
			throw std::invalid_argument(
				std::format("Liquidity must be at least {:.1f}", MIN_LIQUIDITY));
	}

	LMSR::LMSR() : LMSR(DEFAULUT_LIQUIDITY) {}
}

