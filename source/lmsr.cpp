#include "include/lmsr.hpp"

#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <limits>
#include <format>

namespace lmsr {

LMSR::LMSR(double liquidity) : yes_units(0), no_units(0), 
liquidity_(liquidity) {
	if (std::isless(liquidity, MIN_LIQUIDITY))
		throw std::invalid_argument(std::format("Liquidity must be at least {}", MIN_LIQUIDITY));

	if (!std::isfinite(liquidity))
		throw std::invalid_argument("Liquidity must be finite");
}

LMSR::LMSR() : LMSR(DEFAULT_LIQUIDITY) {}

std::int64_t LMSR::to_credit_units(double raw_credits, double (*round_function)(double)) {
	if (std::isless(raw_credits, 0.0))
		throw std::invalid_argument("The amount of unrounded credits must be non-negative");
	if (!std::isfinite(raw_credits))
		throw std::invalid_argument("The amount of unrounded credits must be finite");

	double raw_units = UNITS_PER_CREDIT * raw_credits;

	if (!std::isfinite(raw_units))
		throw std::overflow_error("Unrounded credit units exceed double range");

	double rounded_units = round_function(raw_units);

	const double max_int64 = std::ldexp(1.0, 63);
	if (!std::isless(rounded_units, max_int64))
		throw std::overflow_error("Rounded units exceed int64_t range");

	return static_cast<std::int64_t>(rounded_units);
}

double LMSR::liquidity() const {
	return liquidity_;
}

double LMSR::probability(Outcome outcome) const {
	double q_y = static_cast<double>(yes_units) / UNITS_PER_SHARE;
	double q_n = static_cast<double>(no_units) / UNITS_PER_SHARE;

	double x = (q_y - q_n) / liquidity_;

	if (outcome == Outcome::No)
		x = -x;

	double p;
	if (std::isgreater(x, 0.0)) {
		p = 1.0 / (1.0 + std::exp(-x));
	} else {
		p = std::exp(x) / (std::exp(x) + 1.0);
	}

	return p;
}

double LMSR::cost(std::int64_t y_units, std::int64_t n_units) const {
	double q_y = static_cast<double>(y_units) / UNITS_PER_SHARE;
	double q_n = static_cast<double>(n_units) / UNITS_PER_SHARE;

	double x = -std::abs(q_y - q_n) / liquidity_;
	double c_s = std::max(q_y, q_n) + liquidity_ * std::log1p(std::exp(x));

	double cost = CREDITS_PER_SHARE * c_s;

	if (!std::isfinite(cost))
		throw std::runtime_error("LMSR cost is not finite");

	return cost;
}

double LMSR::cost() const {
	return cost(yes_units, no_units);
}

std::int64_t LMSR::quote_buy(Outcome outcome, std::int64_t share_units) const {
	if (share_units <= 0)
		throw std::invalid_argument("The amount of share units must be positive");

	double p = probability(outcome);

	double a = static_cast<double>(share_units) / UNITS_PER_SHARE;
	double t = a / liquidity_;

	double c_s, delta_c;

	if (std::isless(t, 1.0)) {
		c_s = liquidity_ * std::log1p(p * std::expm1(t));
		delta_c = CREDITS_PER_SHARE * c_s;

		return to_credit_units(delta_c, std::ceil);
	}

	double q_y = static_cast<double>(yes_units) / UNITS_PER_SHARE;
	double q_n = static_cast<double>(no_units) / UNITS_PER_SHARE;

	double x = (q_y - q_n) / liquidity_;
	if (outcome == Outcome::No)
		x = -x;

	double epsilon = std::log1p(std::exp(-std::abs(x)));
	double log_p = -std::max(-x, 0.0) - epsilon, log1mp = -std::max(x, 0.0) - epsilon;

	double u = log_p + t, v = log1mp;
	c_s = liquidity_ * (std::max(u, v) + std::log1p(std::exp(-std::abs(u - v))));

	delta_c = CREDITS_PER_SHARE * c_s;

	return to_credit_units(delta_c, std::ceil);
}

void LMSR::add(Outcome outcome, std::int64_t share_units) {
	std::int64_t y_units = yes_units, n_units = no_units;
	std::int64_t& ref_untis = (outcome == Outcome::Yes ? y_units : n_units);

	if (share_units > std::numeric_limits<std::int64_t>::max() - ref_untis)
		throw std::overflow_error("Share units overflow int64_t range");

	ref_untis += share_units;
}

}

