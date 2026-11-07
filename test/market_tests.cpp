#include "market.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstdint>
#include <limits>
#include <stdexcept>

TEST_CASE("MarketState", "[MarketState]") {
	SECTION("Constructors") {
		SECTION("Reject negative quantities") {
			REQUIRE_THROWS_AS(market::MarketState(-1, 0), std::invalid_argument);
			REQUIRE_THROWS_AS(market::MarketState(0, -1), std::invalid_argument);
			REQUIRE_THROWS_AS(market::MarketState(-1, -1), std::invalid_argument);
		}
	}

	SECTION("quantity") {
		SECTION("Returns zero for a default state") {
			const market::MarketState state;

			CHECK(state.quantity(market::Outcome::Yes) == 0);
			CHECK(state.quantity(market::Outcome::No) == 0);
		}

		SECTION("Returns the units for the requested outcome") {
			const market::MarketState state(1234, 5678);

			CHECK(state.quantity(market::Outcome::Yes) == 1234);
			CHECK(state.quantity(market::Outcome::No) == 5678);
		}
	}

	SECTION("add") {
		SECTION("Increases the requested outcome") {
			market::MarketState state(1234, 5678);

			state.add(market::Outcome::Yes, 123);
			state.add(market::Outcome::No, 3000);

			CHECK(state.quantity(market::Outcome::Yes) == 1357);
			CHECK(state.quantity(market::Outcome::No) == 8678);
		}

		SECTION("Rejects negative units") {
			market::MarketState state(1234, 5678);

			REQUIRE_THROWS_AS(state.add(market::Outcome::Yes, -1), std::invalid_argument);
		}

		SECTION("Rejects overflow") {
			constexpr std::int64_t max_units = std::numeric_limits<std::int64_t>::max();
			market::MarketState state(max_units, 0);

			REQUIRE_THROWS_AS(state.add(market::Outcome::Yes, 1), std::overflow_error);
		}
	}
}

TEST_CASE("LMSR", "[LMSR]") {
	SECTION("Constructors") {
		SECTION("Accepts valid liquidity") {
			REQUIRE_NOTHROW(market::LMSR(25.0));
		}

		SECTION("Accepts default construction") {
			REQUIRE_NOTHROW(market::LMSR{});
		}

		SECTION("Rejects invalid liquidity") {
			REQUIRE_THROWS_AS(market::LMSR(-1.0), std::invalid_argument);
			REQUIRE_THROWS_AS(market::LMSR(0.5), std::invalid_argument);
			REQUIRE_THROWS_AS(market::LMSR(std::numeric_limits<double>::quiet_NaN()), std::invalid_argument);
			REQUIRE_THROWS_AS(market::LMSR(std::numeric_limits<double>::infinity()), std::invalid_argument);
		}
	}

	SECTION("liquidity") {
		const market::LMSR lmsr(25.0);

		CHECK(lmsr.liquidity() == 25.0);
	}

	SECTION("probability") {
		constexpr double epsilon = 1e-12;
		using Catch::Matchers::WithinAbs;

		SECTION("Balanced quantities give equal probabilities") {
			const market::LMSR lmsr(25.0);
			const market::MarketState state(1234, 1234);

			CHECK_THAT(lmsr.probability(state, market::Outcome::Yes), WithinAbs(0.5, epsilon));
			CHECK_THAT(lmsr.probability(state, market::Outcome::No), WithinAbs(0.5, epsilon));
		}

		SECTION("One share unit affects probabilities") {
			const market::LMSR lmsr(1.0);
			const market::MarketState state(1, 0);

			CHECK_THAT(lmsr.probability(state, market::Outcome::Yes), WithinAbs(0.5002499999791667, epsilon));
			CHECK_THAT(lmsr.probability(state, market::Outcome::No), WithinAbs(0.4997500000208333, epsilon));
		}

		SECTION("Extreme imbalances approach zero and one") {
			const market::LMSR lmsr(1.0);
			const market::MarketState yes_heavy(1000000, 0);
			const market::MarketState no_heavy(0, 1000000);

			CHECK_THAT(lmsr.probability(yes_heavy, market::Outcome::Yes), WithinAbs(1.0, epsilon));
			CHECK_THAT(lmsr.probability(yes_heavy, market::Outcome::No), WithinAbs(0.0, epsilon));
			CHECK_THAT(lmsr.probability(no_heavy, market::Outcome::Yes), WithinAbs(0.0, epsilon));
			CHECK_THAT(lmsr.probability(no_heavy, market::Outcome::No), WithinAbs(1.0, epsilon));
		}

		SECTION("Regular cases match pre-calculated probabilities") {
			struct ProbabilityCase {
				std::int64_t yes_units;
				std::int64_t no_units;
				double liquidity;
				double expected_yes;
				double expected_no;
			};

			// Quantities are share units; reference probabilities use quantities in shares.
			constexpr ProbabilityCase cases[] = {
				{1000, 0, 1.0, 0.7310585786300049, 0.2689414213699951},
				{0, 1000, 1.0, 0.2689414213699951, 0.7310585786300049},
				{3000, 1000, 1.0, 0.8807970779778823, 0.1192029220221176},
				{1000, 3000, 1.0, 0.1192029220221176, 0.8807970779778823},
				{1500, 1000, 1.0, 0.6224593312018546, 0.3775406687981454},
				{1000, 1500, 1.0, 0.3775406687981454, 0.6224593312018546},
				{1100, 1000, 1.0, 0.5249791874789400, 0.4750208125210600},
				{1000, 1100, 1.0, 0.4750208125210600, 0.5249791874789400},
				{11000, 1000, 10.0, 0.7310585786300049, 0.2689414213699951},
				{1000, 11000, 10.0, 0.2689414213699951, 0.7310585786300049},
				{50000, 25000, 25.0, 0.7310585786300049, 0.2689414213699951},
				{25000, 50000, 25.0, 0.2689414213699951, 0.7310585786300049},
				{10000, 0, 100.0, 0.5249791874789400, 0.4750208125210600},
				{0, 10000, 100.0, 0.4750208125210600, 0.5249791874789400},
				{1250, 1000, 25.0, 0.5024999791668750, 0.4975000208331250},
				{1000, 1250, 25.0, 0.4975000208331250, 0.5024999791668750},
			};

			for (const auto& test : cases) {
				CAPTURE(test.yes_units, test.no_units, test.liquidity);
				const market::LMSR lmsr(test.liquidity);
				const market::MarketState state(test.yes_units, test.no_units);

				CHECK_THAT(lmsr.probability(state, market::Outcome::Yes), WithinAbs(test.expected_yes, epsilon));
				CHECK_THAT(lmsr.probability(state, market::Outcome::No), WithinAbs(test.expected_no, epsilon));
			}
		}
	}
}
