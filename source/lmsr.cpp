#include "lmsr.hpp"

#include <stdexcept>
#include <cmath>
#include <algorithm>

namespace lmsr {

	LMSR::LMSR(double liquidity) : yes_share_units(0), no_share_units(0), 
	liquidity(liquidity) {
		if (!std::isgreater(liquidity, 0.0))
			throw std::invalid_argument("Liquidity must be positive");

		if (!std::isfinite(liquidity))
			throw std::invalid_argument("Liquidity must be finite");
	}

	LMSR::LMSR() : LMSR(DEFAULT_LIQUIDITY) {}

	double LMSR::price(Outcome outcome) const {
		double q_y = static_cast<double>(yes_share_units) / UNITS_PER_SHARE;
		double q_n = static_cast<double>(no_share_units) / UNITS_PER_SHARE;

		double x = (q_y - q_n) / liquidity;
		double p_y;

		if (std::isgreater(x, 0.0)) {
			p_y = 1.0 / (1.0 + std::exp(-x));
		} else {
			p_y = std::exp(x) / (std::exp(x) + 1.0);
		}

		if (outcome == Outcome::Yes) return p_y;
		else return 1 - p_y;
	}

	double LMSR::cost() const {
		double q_y = static_cast<double>(yes_share_units) / UNITS_PER_SHARE;
		double q_n = static_cast<double>(no_share_units) / UNITS_PER_SHARE;

		double x = -std::abs(q_y - q_n) / liquidity;
		double c_s = std::max(q_y, q_n) + liquidity * std::log1p(std::exp(x));

		return CREDITS_PER_SHARE * c_s;
	}
}

