#pragma once
#include <array>
#include "util/are_same_v.hpp"

template<typename T, typename... As>
	requires (are_same_v<T, std::remove_reference_t<As>> && ...)
constexpr auto make_array(As... as)
	-> std::array<T, sizeof...(As)>
{
	return {
		std::forward<As>(as)...
	};
}
