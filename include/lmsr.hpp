#pragma once

#include <cstdint>

namespace lmsr {

enum class Outcome { Yes, No };

struct MarketState
{
	std::int64_t yes_share_units = 0;
	std::int64_t no_share_units = 0;
	std::int64_t liquidity_units = 100'000;
};

double cost(const MarketState& market);

double price(const MarketState& market, Outcome outcome);

std::int64_t quote_buy(
	const MarketState& market, 
	Outcome outcome, 
	std::int64_t quantity
);

}