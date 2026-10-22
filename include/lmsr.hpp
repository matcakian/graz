#pragma once

#include <cstdint>

namespace lmsr {
enum class Outcome {Yes, No};

class LMSR {
private:
	static constexpr double DEFAULT_LIQUIDITY = 100.0;
	static constexpr double MIN_LIQUIDITY = 1.0;

	std::int64_t yes_units_;
	std::int64_t no_units_;

	double liquidity_;

	static std::int64_t to_credit_units(double raw_credits, double (*f)(double));

public:
	static constexpr std::int64_t UNITS_PER_SHARE = 1000;
	static constexpr std::int64_t UNITS_PER_CREDIT = 1000;
	static constexpr std::int64_t CREDITS_PER_SHARE = 1;

	explicit LMSR(double liquidity);
	LMSR();

	[[nodiscard]] std::int64_t share_units(Outcome outcome) const;

	[[nodiscard]] double liquidity() const;
	[[nodiscard]] double probability(Outcome outcome) const;
	[[nodiscard]] double cost(std::int64_t y_units, std::int64_t n_units) const;
	[[nodiscard]] double cost() const;

	[[nodiscard]] std::int64_t quote_buy(Outcome outcome, std::int64_t share_units) const;

	void add(Outcome outcome, std::int64_t share_units);
};
}