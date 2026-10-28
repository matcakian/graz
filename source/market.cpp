#include "market.hpp"

#include <cstdint>
#include <stdexcept>

namespace market {
	MarketState::MarketState(std::int64_t y_units, std::int64_t n_units) :
	y_units_(y_units), n_units_(n_units) {
		if (y_units < 0 || n_units < 0)
			throw std::invalid_argument("Share units must be positive");
	}

	MarketState::MarketState() : MarketState(0, 0) {}
}