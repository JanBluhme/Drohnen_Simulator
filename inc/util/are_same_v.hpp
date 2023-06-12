#pragma once

template<typename... Ts>
inline constexpr bool are_same_v = true;

template<typename T, typename... Ts>
inline constexpr bool are_same_v<T, Ts...>
	= (std::is_same_v<T, Ts> && ...)
;
