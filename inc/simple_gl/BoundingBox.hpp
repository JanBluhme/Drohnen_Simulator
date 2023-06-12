#pragma once
#include "math/Vertex.hpp"
#include <limits>
#include <algorithm>

struct BoundingBox {
	Vertex<double, 3> min {
		  std::numeric_limits<double>::max()
		, std::numeric_limits<double>::max()
		, std::numeric_limits<double>::max()
	};
	Vertex<double, 3> max = -min;

	void grow(Vertex<double, 3> const& v) {
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((min[Is] = std::min(min[Is], v[Is])), ...);
			((max[Is] = std::max(max[Is], v[Is])), ...);
		}(std::make_index_sequence<3>{});
	}
	constexpr auto center() const noexcept
		-> Vertex<double, 3>
	{
		return (min + max) / 2.0;
	}
	constexpr auto size() const noexcept
		-> Vertex<double, 3>
	{
		return max - min;
	}
	constexpr auto contains(Vertex<double, 3> const& p) const noexcept
		-> bool
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return ((p[Is] >= min[Is] && p[Is] <= max[Is]) && ...);
		}(std::make_index_sequence<3>{});
	}
};
