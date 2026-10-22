#include "lmsr.hpp"
#include <catch2/catch_test_macros.hpp>


TEST_CASE("Default liquidity is positive", "[constructor]") {
	lmsr::LMSR engine;

	REQUIRE(engine.liquidity() > 0);
}
