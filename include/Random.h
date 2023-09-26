//
// Created by byrax on 2023-09-26.
//

#pragma once
#include <random>

namespace Random {
	inline std::random_device rd;
	inline std::mt19937		  gen{ rd() };

	template <typename DistribImpl>
	class Distribution {
	private:
		mutable DistribImpl distribution;

	public:
		explicit Distribution(DistribImpl&& distrib)
			: distribution{ distrib } {}

		auto operator()() const { return distribution(gen); }
	};
}
