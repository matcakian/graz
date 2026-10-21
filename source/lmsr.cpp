#include "lmsr.hpp"

#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <limits>

namespace lmsr {

std::int64_t LMSR::to_credit_units(double r_credits, double (*f)(double)) {
	double r_units = UNITS_PER_CREDIT * r_credits;

	if (std::isless(r_units, 0.0))
		throw std::invalid_argument("The amount of units must be non-negative");
	if (!std::isfinite(r_units))
		throw std::invalid_argument("The amount of units must be finite");

	const double max_int64 = std::ldexp(1.0, 63);
	if (f(r_units) > max_int64)
		throw std::overflow_error("Rounded units exceed int64_t range");

	return static_cast<int64_t>(f(r_units));
}

LMSR::LMSR(double liquidity) : yes_units(0), no_units(0), 
liquidity(liquidity) {
	if (!std::isgreater(liquidity, 0.0))
		throw std::invalid_argument("Liquidity must be positive");

	if (!std::isfinite(liquidity))
		throw std::invalid_argument("Liquidity must be finite");
}

LMSR::LMSR() : LMSR(DEFAULT_LIQUIDITY) {}

double LMSR::price(Outcome outcome) const {
	double q_y = static_cast<double>(yes_units) / UNITS_PER_SHARE;
	double q_n = static_cast<double>(no_units) / UNITS_PER_SHARE;

	double x = (q_y - q_n) / liquidity;
	double p_y;

	if (std::isgreater(x, 0.0)) {
		p_y = 1.0 / (1.0 + std::exp(-x));
	} else {
		p_y = std::exp(x) / (std::exp(x) + 1.0);
	}

	if (outcome == Outcome::Yes)
		return to_credit_units(CREDITS_PER_SHARE * p_y);
	else
		return to_credit_units(CREDITS_PER_SHARE * (1 - p_y));
}

double LMSR::cost(std::int64_t y_units, std::int64_t n_units) const {
	double q_y = static_cast<double>(y_units) / UNITS_PER_SHARE;
	double q_n = static_cast<double>(n_units) / UNITS_PER_SHARE;

	double x = -std::abs(q_y - q_n) / liquidity;
	double c_s = std::max(q_y, q_n) + liquidity * std::log1p(std::exp(x));

	double cost = CREDITS_PER_SHARE * c_s;

	if (!std::isfinite(cost))
		throw std::runtime_error("LMSR cost is not finite");

	return cost;
}

double LMSR::cost() const {
	return cost(yes_units, no_units);
}

std::int64_t LMSR::quote_buy(Outcome outcome, std::int64_t quantity) const {
	if (quantity <= 0)
		throw std::invalid_argument("The amount of share units must be positive");

	int64_t y_units = yes_units, n_units = no_units;
	int64_t& ref_units = (outcome == Outcome::Yes ? y_units : n_units);

	if (quantity > std::numeric_limits<std::int64_t>::max() - ref_units)
		throw std::overflow_error("Share units overflow");

	ref_units += quantity;

	double delta_c = cost(y_units, n_units) - cost();

	return to_credit_units(delta_c, std::ceil);
}
}

