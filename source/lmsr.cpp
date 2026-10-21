#include "lmsr.hpp"

#include <stdexcept>
#include <cmath>

namespace lmsr {

	class LMSR {
	public:
		explicit LMSR(double liquidity) : yes_share_units(0), no_share_units(0), 
		liquidity(liquidity) {
			if (!std::isgreater(liquidity, 0.0))
				throw std::invalid_argument("Liquidity must be positive");

			if (!std::isfinite(liquidity))
				throw std::invalid_argument("Liquidity must be finite")
		}

		LMSR() : LMSR(DEFAULT_LIQUIDITY) {}

	};
}
