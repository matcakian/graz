#include "market.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>
#include <limits>
#include <stdexcept>

TEST_CASE("MarketState constructor rejects negative share counts", "[market]") {
	CHECK_THROWS_AS(market::MarketState(-1, 0), std::invalid_argument);
	CHECK_THROWS_AS(market::MarketState(0, -1), std::invalid_argument);
}

TEST_CASE("MarketState quantity returns the share count for each outcome", "[market]") {
	const market::MarketState state(10, 20);

	CHECK(state.quantity(market::Outcome::Yes) == 10);
	CHECK(state.quantity(market::Outcome::No) == 20);
}

TEST_CASE("MarketState add", "[market]") {
	SECTION("Increases the selected outcome's quantity") {
		market::MarketState state(10, 20);

		state.add(market::Outcome::Yes, 5);
		CHECK(state.quantity(market::Outcome::Yes) == 15);
		CHECK(state.quantity(market::Outcome::No) == 20);

		state.add(market::Outcome::No, 7);
		CHECK(state.quantity(market::Outcome::Yes) == 15);
		CHECK(state.quantity(market::Outcome::No) == 27);
	}

	SECTION("Accepts zero shares") {
		market::MarketState state(10, 20);

		state.add(market::Outcome::Yes, 0);
		CHECK(state.quantity(market::Outcome::Yes) == 10);
		CHECK(state.quantity(market::Outcome::No) == 20);
	}

	SECTION("Rejects negative share counts") {
		market::MarketState state(10, 20);

		CHECK_THROWS_AS(state.add(market::Outcome::Yes, -1), std::invalid_argument);
		CHECK(state.quantity(market::Outcome::Yes) == 10);
		CHECK(state.quantity(market::Outcome::No) == 20);
	}

	SECTION("Rejects quantities beyond the int64 limit") {
		constexpr auto MAX_QUANTITY = std::numeric_limits<std::int64_t>::max();
		market::MarketState state(10, MAX_QUANTITY - 1);

		state.add(market::Outcome::No, 1);
		REQUIRE(state.quantity(market::Outcome::No) == MAX_QUANTITY);

		CHECK_THROWS_AS(state.add(market::Outcome::No, 1), std::overflow_error);
		CHECK(state.quantity(market::Outcome::Yes) == 10);
		CHECK(state.quantity(market::Outcome::No) == MAX_QUANTITY);
	}
}

TEST_CASE("LMSR constructor", "[lmsr]") {
	SECTION("Uses the default liquidity") {
		const market::LMSR lmsr;
		CHECK(lmsr.liquidity() == market::LMSR::DEFAULT_LIQUIDITY);
	}

	SECTION("Rejects liquidity below the minimum") {
		CHECK_THROWS_AS(market::LMSR(0.5), std::invalid_argument);
		CHECK_THROWS_AS(market::LMSR(0.0), std::invalid_argument);
		CHECK_THROWS_AS(market::LMSR(-1.0), std::invalid_argument);
	}

	SECTION("Rejects non-finite liquidity") {
		constexpr auto INFINITY = std::numeric_limits<double>::infinity();
		constexpr auto NaN = std::numeric_limits<double>::quiet_NaN();

		CHECK_THROWS_AS(market::LMSR(INFINITY), std::invalid_argument);
		CHECK_THROWS_AS(market::LMSR(-INFINITY), std::invalid_argument);
		CHECK_THROWS_AS(market::LMSR(NaN), std::invalid_argument);
	}
}

TEST_CASE("LMSR liquidity returns the configured value", "[lmsr]") {
	const market::LMSR lmsr(25.5);

	CHECK(lmsr.liquidity() == 25.5);
}

TEST_CASE("LMSR probability", "[lmsr]") {
	constexpr double ABS_TOLERANCE = 1e-12;

	SECTION("Assigns equal probabilities to equal quantities") {
		const market::LMSR lmsr(15.0);
		const market::MarketState state(10, 10);

		CHECK(lmsr.probability(state, market::Outcome::Yes) == 0.5);
		CHECK(lmsr.probability(state, market::Outcome::No) == 0.5);
	}

	SECTION("Matches expected probabilities across market states and liquidities") {
		struct ProbabilityCase {
			std::int64_t yes_shares;
			std::int64_t no_shares;
			double liquidity;
			double expected_yes;
			double expected_no;
		};

		constexpr ProbabilityCase cases[] = {
			// Yes shares  No shares  Liquidity  Expected P(Yes)        Expected P(No)
			{0,            20,        1.0,       2.061153618190204e-9,  0.9999999979388463},
			{0,            100,       10.0,      0.0000453978687024,    0.9999546021312976},
			{15,           65,        10.0,      0.0066928509242849,    0.9933071490757153},
			{8,            28,        10.0,      0.1192029220221175,    0.8807970779778823},
			{40,           50,        10.0,      0.2689414213699951,    0.7310585786300049},
			{15,           20,        2.5,       0.1192029220221175,    0.8807970779778823},
			{20,           15,        1.0,       0.9933071490757153,    0.0066928509242849},
			{20,           15,        2.5,       0.8807970779778823,    0.1192029220221175},
			{20,           15,        10.0,      0.6224593312018546,    0.3775406687981454},
			{20,           15,        100.0,     0.5124973964842103,    0.4875026035157896},
			{43,           13,        10.0,      0.9525741268224334,    0.0474258731775668},
			{70,           0,         10.0,      0.9990889488055994,    0.0009110511944006},
			{101,          1,         10.0,      0.9999546021312976,    0.0000453978687024},
			{101,          1,         250.0,     0.5986876601124520,    0.4013123398875480},
			{20,           15,        1e6,       0.5000012500000000,    0.4999987500000000},
			{1'000'000,    0,         1e6,       0.7310585786300049,    0.2689414213699951},
			{0,            100,       1e9,       0.4999999750000001,    0.5000000250000000},
			{101,          1,         1e12,      0.5000000000250000,    0.4999999999750000},
		};

		for (const auto& [yes_shares, no_shares, liquidity, expected_yes, expected_no] : cases) {
			DYNAMIC_SECTION("Yes shares " << yes_shares << ", No shares " << no_shares
				<< ", liquidity " << liquidity) {
				const market::MarketState state(yes_shares, no_shares);
				const market::LMSR lmsr(liquidity);

				CHECK_THAT(lmsr.probability(state, market::Outcome::Yes),
					Catch::Matchers::WithinAbs(expected_yes, ABS_TOLERANCE));
				CHECK_THAT(lmsr.probability(state, market::Outcome::No),
					Catch::Matchers::WithinAbs(expected_no, ABS_TOLERANCE));
			}
		}
	}
}
