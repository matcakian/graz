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

	bool MarketState::add(Outcome outcome, std::int64_t s_units) {
		// referenced units
		int64_t& r_units = (outcome == Outcome::Yes ? y_units_ : n_units_);

		constexpr std::int64_t MAX = std::numeric_limits<std::int64_t>::max();
		if (s_units > MAX - r_units)
			return false;

		r_units += s_units;
		return true;
	}

	LMSR::LMSR(double liquidity) : liquidity_(liquidity) {
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
		double q_y = state.quantity(Outcome::Yes);
		double q_n = state.quantity(Outcome::No);
		double b = liquidity_;

		double x = (q_y - q_n) / b;
		if (outcome == Outcome::No) x = -x;

		double result;
		if (std::isless(x, 0.0))
			result = std::exp(x) / (std::exp(x) + 1.0);
		else
			result = 1.0 / (1.0 + std::exp(-x));

		return result;
	}

	std::int64_t to_credit_units(double r_credits, double (*f)(double)) {
		if (std::isnan(r_credits)) 
			throw std::invalid_argument("Raw credits must not be NAN");

		double r_units = r_credits * UNITS_PER_CREDIT; // raw units
		if (!std::isfinite(r_units))
			throw std::overflow_error("Raw units overflow double range");

		double result = f(r_units);

		constexpr double MAX = static_cast<double>(std::numeric_limits<std::int64_t>::max());
		if (std::isgreater(result, MAX))
			throw std::overflow_error("Rounded units overflow std::int64_t range");

		return result;
	}
}

