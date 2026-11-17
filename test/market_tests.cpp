#include "market.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <algorithm>
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
		CHECK(lmsr.liquidity() == 100.0);
	}

	SECTION("Rejects liquidity below the minimum") {
		CHECK_THROWS_AS(market::LMSR(0.5), std::invalid_argument);
		CHECK_THROWS_AS(market::LMSR(0.0), std::invalid_argument);
		CHECK_THROWS_AS(market::LMSR(-1.0), std::invalid_argument);
	}

	SECTION("Rejects non-finite liquidity") {
		constexpr auto POSITIVE_INFINITY = std::numeric_limits<double>::infinity();
		constexpr auto NaN = std::numeric_limits<double>::quiet_NaN();

		CHECK_THROWS_AS(market::LMSR(POSITIVE_INFINITY), std::invalid_argument);
		CHECK_THROWS_AS(market::LMSR(-POSITIVE_INFINITY), std::invalid_argument);
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
			DYNAMIC_SECTION("Yes shares: " << yes_shares << "; No shares: " << no_shares
				<< "; Liquidity: " << liquidity) {
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

TEST_CASE("LMSR quote_buy", "[lmsr]") {
	SECTION("Rejects non-positive share amounts") {
		const market::LMSR lmsr;
		const market::MarketState state(10, 20);
		CHECK_THROWS_AS(lmsr.quote_buy(state, market::Outcome::Yes, 0), std::invalid_argument);
		CHECK_THROWS_AS(lmsr.quote_buy(state, market::Outcome::Yes, -1), std::invalid_argument);
		CHECK_THROWS_AS(lmsr.quote_buy(state, market::Outcome::No,
				std::numeric_limits<std::int64_t>::min()), std::invalid_argument);
	}

	SECTION("Quotes never undercharge and respect the overcharge and payout limits") {
		// Allow the larger of one credit and one part per million of the minimum price for rounding.
		constexpr std::int64_t OVERCHARGE_DIVISOR = 1'000'000;
		constexpr std::int64_t ABS_ERROR = 1;

		struct QuoteCase {
			std::int64_t yes_shares;
			std::int64_t no_shares;
			double liquidity;
			std::int64_t shares;
			std::int64_t minimum_yes;
			std::int64_t minimum_no;
		};

		// Minimum quotes are the LMSR cost differences rounded up to whole credits.
		constexpr QuoteCase cases[] = {
			// Yes shares                No shares                   Liquidity  Buy shares  Minimum Yes  Minimum No
			{0,                          0,                          1.0,       1,          1,           1},
			{0,                          0,                          1.0,       2,          2,           2},
			{0,                          0,                          1.0,       3,          3,           3},
			{0,                          0,                          100.0,     100,        63,          63},
			{0,                          0,                          1e12,      2,          2,           2},
			{0,                          0,                          1e12,      1'000'000,  500'001,     500'001},
			{0,                          1,                          1.0,       1,          1,           1},
			{0,                          1,                          1.0,       2,          1,           2},
			{0,                          1,                          1.0,       3,          2,           3},
			{1,                          0,                          1.25,      10,         10,          9},
			{1,                          2,                          2.5,       9,          7,           8},
			{2,                          10,                         2.5,       11,         4,           11},
			{10,                         2,                          25.5,      49,         38,          32},
			{10,                         100,                        25.5,      50,         5,           50},
			{100,                        10,                         25.5,      51,         51,          5},
			{10,                         100,                        1e3,       100,        50,          54},
			{15,                         20,                         2.5,       8,          4,           8},
			{25,                         25,                         10.0,      15,         11,          11},
			{40,                         75,                         10.0,      20,         2,           20},
			{75,                         40,                         10.0,      35,         35,          7},
			{0,                          100,                        1.0,       99,         1,           99},
			{0,                          100,                        1.0,       100,        1,           100},
			{0,                          100,                        1.0,       101,        2,           101},
			{0,                          1'000,                      1.0,       999,        1,           999},
			{0,                          1'000,                      1.0,       1'000,      1,           1'000},
			{0,                          1'000,                      1.0,       1'001,      2,           1'001},
			{0,                          1'000,                      10.0,      999,        7,           999},
			{0,                          1'000,                      10.0,      1'000,      7,           1'000},
			{0,                          1'000,                      10.0,      1'001,      8,           1'001},
			{100,                        1'000,                      100.0,     1'000,      132,         1'000},
			{1'000,                      100,                        1e6,       100'000,    51'272,      51'227},
			{125,                        175,                        25.5,      100,        50,          97},
			{250,                        250,                        100.0,     75,         45,          45},
			{255,                        256,                        1.25,      2,          1,           2},
			{320,                        470,                        50.0,      300,        150,         298},
			{900,                        1'000,                      100.0,     150,        67,          127},
			{1'250,                      1'800,                      250.0,     700,        234,         676},
			{4'096,                      4'095,                      100.0,     150,        102,         101},
			{12'000,                     18'500,                     5e3,       4'000,      1'166,       3'373},
			{62'000,                     58'000,                     2.5e3,     9'000,      8'555,       4'858},
			{125'000,                    275'000,                    1e5,       225'000,    93'546,      207'184},
			{500'000,                    500'000,                    1e6,       100'000,    51'250,      51'250},
			{800'000,                    800'001,                    100.0,     200,        143,         144},
			{1'000'000,                  0,                          1.0,       1,          1,           1},
			{1'000'000,                  0,                          1.0,       1'000'000,  1'000'000,   1},
			{1'000'000,                  0,                          1e6,       1'000'000,  813'667,     379'886},
			{1'250'000,                  1'700'000,                  1e5,       500'000,    96'303,      498'903},
			{3'000'000,                  3'000'000,                  100.0,     2'000'000,  1'999'931,   1'999'931},
			{45'000'000,                 57'000'000,                 1e7,       1'000'000,  240'529,     777'260},
			{999'999'999,                1'000'000'000,              25.5,      50,         36,          37},
			{1'000'000'000'001,          1'000'000'000'000,          1e9,       100'000,    50'002,      50'002},
			{9'007'199'254'740'991,      9'007'199'254'740'992,      1.0,       2,          1,           2},
			{9'007'199'254'740'992,      9'007'199'254'740'993,      1.0,       2,          1,           2},
			{9'007'199'254'740'993,      9'007'199'254'740'991,      2.5,       3,          3,           2},
			{9'007'199'254'740'993,      9'007'199'254'740'993,      1e12,      1'000'000,  500'001,     500'001},
			{9'223'372'036'853'775'807,  0,                          1.0,       1,          1,           1},
			{9'223'372'036'853'775'807,  1'000'000,                  100.0,     100'000,    100'000,     1},
			{9'223'372'036'853'775'807,  9'223'372'036'853'775'807,  100.0,     1'000'000,  999'931,     999'931},
		};

		for (const auto& [yes_shares, no_shares, liquidity, shares, minimum_yes, minimum_no] : cases) {
			DYNAMIC_SECTION("Yes shares: " << yes_shares << "; No shares: " << no_shares
				<< "; Liquidity: " << liquidity << "; Buy shares: " << shares) {
				const market::MarketState state(yes_shares, no_shares);
				const market::LMSR lmsr(liquidity);
				const auto max_payout = shares; // Each share pays one credit.
				const auto yes_quote = lmsr.quote_buy(state, market::Outcome::Yes, shares);
				const auto no_quote = lmsr.quote_buy(state, market::Outcome::No, shares);

				CHECK(yes_quote >= minimum_yes);
				CHECK(yes_quote <= max_payout);
				CHECK(yes_quote <= minimum_yes + std::max(minimum_yes / OVERCHARGE_DIVISOR, ABS_ERROR));

				CHECK(no_quote >= minimum_no);
				CHECK(no_quote <= max_payout);
				CHECK(no_quote <= minimum_no + std::max(minimum_no / OVERCHARGE_DIVISOR, ABS_ERROR));
			}
		}
	}
}
