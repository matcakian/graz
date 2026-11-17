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

void MarketState::add(Outcome outcome, std::int64_t shares) {
	if (shares < 0)
		throw std::invalid_argument("The number of shares must be positive");

	// Check the remaining capacity before adding to avoid signed overflow.
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
	// Subtract as integers to preserve small share gaps when computing log-odds.
	const double x = static_cast<double>(state.quantity(outcome)
		- state.quantity(!outcome)) / liquidity_;
	
	// Use a nonpositive exponent in both forms to avoid overflow.
	if (std::isless(x, 0.0))
		return std::exp(x) / (std::exp(x) + 1.0);
	else
		return 1.0 / (1.0 + std::exp(-x));
}

namespace {

constexpr double INF = std::numeric_limits<double>::infinity();

// Use std::nextafter to widen intervals for floating-point rounding.
struct Bound {
	double lower, upper;

	Bound(double lower, double upper) : lower(lower), upper(upper) {
		assert(std::isfinite(lower) && std::isfinite(upper));
		assert(std::isgreaterequal(upper, lower));
	}

	Bound(std::int64_t value) {
		double fp_value = static_cast<double>(value);

		// A double represents every integer from -2^53 through 2^53 exactly.
		constexpr std::int64_t MAX_REPR = 1ll << 53;

		if (-MAX_REPR <= value && value <= MAX_REPR) {
			this->lower = fp_value;
			this->upper = fp_value;
		} else if (value >= 0) {
			this->lower = std::nextafter(fp_value, 0.0);
			this->upper = std::nextafter(fp_value, INF);
		} else {
			this->lower = std::nextafter(fp_value, -INF);
			this->upper = std::nextafter(fp_value, 0.0);
		}
	}

	Bound(double value) : Bound(value, value) {}

	Bound() : Bound(0.0) {}

	static Bound exp(const Bound& bound) {
		double lower = std::exp(bound.lower);
		double upper = std::exp(bound.upper);

		return {std::nextafter(lower, 0.0), std::nextafter(upper, INF)};
	}

	static Bound log1p(const Bound& bound) {
		assert(std::isgreater(bound.lower, -1.0));

		double lower = std::log1p(bound.lower);
		double upper = std::log1p(bound.upper);

		return {std::nextafter(lower, -INF), std::nextafter(upper, INF)};
	}

	static Bound expm1(const Bound& bound) {
		double lower = std::expm1(bound.lower);
		double upper = std::expm1(bound.upper);

		return {std::nextafter(lower, -1.0), std::nextafter(upper, INF)};
	}

	static Bound max(const Bound& a, const Bound& b) {
		return {std::max(a.lower, b.lower), std::max(a.upper, b.upper)};
	}

	static Bound abs(const Bound& bound) {
		double lower = std::abs(bound.lower);
		double upper = std::abs(bound.upper);

		return {std::max(std::min(lower, upper), 0.0), std::max(lower, upper)};
	}
};

Bound operator/(const Bound& a, const Bound& b) {
	assert(std::isless(b.upper, 0.0) || std::isgreater(b.lower, 0.0));

	const auto [lower, upper] = std::minmax({
		a.lower / b.lower,
		a.lower / b.upper,
		a.upper / b.lower,
		a.upper / b.upper,
	});

	return {std::nextafter(lower, -INF), std::nextafter(upper, INF)};
}

Bound operator-(const Bound& a, const Bound& b) {
	double lower = a.lower - b.upper;
	double upper = a.upper - b.lower;
	return {std::nextafter(lower, -INF), std::nextafter(upper, INF)};
}

Bound operator-(const Bound& bound) {
	return {-bound.upper, -bound.lower};
}

Bound operator+(const Bound& a, const Bound& b) {
	double lower = a.lower + b.lower;
	double upper = a.upper + b.upper;
	return {std::nextafter(lower, -INF), std::nextafter(upper, INF)};
}

Bound operator*(const Bound& a, const Bound& b) {
	const auto [lower, upper] = std::minmax({
		a.lower * b.lower,
		a.lower * b.upper,
		a.upper * b.lower,
		a.upper * b.upper,
	});

	return {std::nextafter(lower, -INF), std::nextafter(upper, INF)};
}

// The logistic function is increasing, so its bounds come from the endpoints.
Bound p(const Bound& x_b) {
	const auto at = [](double x) -> Bound {
		if (std::isless(x, 0.0)) {
			Bound e = Bound::exp(Bound(x));
			return e / (e + Bound(1.0));
		} else {
			Bound e = Bound::exp(Bound(-x));
			return Bound(1.0) / (Bound(1.0) + e);
		}
	};

	return {std::min(at(x_b.lower).lower, at(x_b.upper).lower),
			std::max(at(x_b.lower).upper, at(x_b.upper).upper)};
}

// Both cost formulas evaluate b * log((1 - p(x)) + p(x) * exp(t)).
// log1p and expm1 preserve precision for small trades.
Bound f_b(const Bound& t_b, const Bound& x_b, const Bound& b_b) {
	return b_b * Bound::log1p(p(x_b) * Bound::expm1(t_b));
}

// Log-sum-exp avoids exponentiating the full trade size.
Bound g_b(const Bound& t_b, const Bound& x_b, const Bound& b_b) {
	// Compute both log probabilities directly to retain tiny probabilities.
	const Bound log_p_b = -Bound::max(-x_b, Bound(0.0)) -
		Bound::log1p(Bound::exp(-Bound::abs(x_b)));
	const Bound log_1mp_b = -Bound::max(x_b, Bound(0.0)) -
		Bound::log1p(Bound::exp(-Bound::abs(x_b)));

	const Bound u_b = log_p_b + t_b, v_b = log_1mp_b;

	return b_b * (Bound::max(u_b, v_b) +
		Bound::log1p(Bound::exp(-Bound::abs(u_b - v_b))));
}

} // namespace

double LMSR::cost_difference(const MarketState& state, Outcome outcome,
	std::int64_t shares) const {
	// Normalize the trade size and the exact share gap by liquidity.
	const Bound b_b = Bound(liquidity_);
	const Bound t_b = Bound(shares) / b_b;

	const Bound x_b = Bound(state.quantity(outcome) - state.quantity(!outcome)) / b_b;

	const auto at = [&x_b, &b_b](double t) -> Bound {
		constexpr double CROSSOVER_T = 100.0;

		if (std::isless(t, CROSSOVER_T))
			return f_b(Bound(t), x_b, b_b);
		else
			return g_b(Bound(t), x_b, b_b);
	};

	// Round the payout cap outward to cover conversion to double.
	const double max_cost = std::nextafter(static_cast<double>(shares), INF);

	// Check both endpoints because the trade-size interval can cross the formula cutoff.
	return std::min(std::max(at(t_b.lower).upper, at(t_b.upper).upper), max_cost);
}

std::int64_t LMSR::quote_buy(const MarketState& state, Outcome outcome,
	std::int64_t shares) const {
	if (shares <= 0)
		throw std::invalid_argument("The number of shares must be positive");

	// Round up to a whole credit.
	const double cost = std::ceil(cost_difference(state, outcome, shares));

	assert(std::isfinite(cost));

	// Use the payout cap when the cost exceeds the largest double below 2^63.
	constexpr double MAX_INT64_D = 9'223'372'036'854'774'784.0;
	if (!std::islessequal(cost, MAX_INT64_D))
		return shares;

	return static_cast<std::int64_t>(cost);
}

} // namespace market

