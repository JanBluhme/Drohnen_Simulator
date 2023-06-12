#pragma once
#include <utility>
#include <cstdint>

template<std::size_t S>
struct unsigned_type {};
template<> struct unsigned_type<sizeof(uint8_t )> { using type = uint8_t;  };
template<> struct unsigned_type<sizeof(uint16_t)> { using type = uint16_t; };
template<> struct unsigned_type<sizeof(uint32_t)> { using type = uint32_t; };
template<> struct unsigned_type<sizeof(uint64_t)> { using type = uint64_t; };

template<std::size_t S>
using unsigned_type_t = typename unsigned_type<S>::type;


struct EndianSwapper {
	template<typename T, std::size_t... N>
	static constexpr T byte_swap_impl(T x, std::index_sequence<N...>) {
		return (
			  (((x >> (N * 8)) & T{255}) << ((sizeof(T) - 1 - N) * 8))
			| ...
		);
	}
	template<typename T>
	static constexpr T byte_swap(T x) {
		if constexpr(std::is_integral_v<T>) {
			return static_cast<T>(
				byte_swap_impl(
					  static_cast<std::make_unsigned_t<T>>(x)
					, std::make_index_sequence<sizeof(T)>{}
				)
			);
		} else if constexpr(std::is_enum_v<T>) {
			return static_cast<T>(
				byte_swap(static_cast<std::underlying_type_t<T>>(x))
			);
		} else if constexpr(std::is_floating_point_v<T>) {
			return static_cast<T>(
				byte_swap(
					static_cast<unsigned_type_t<sizeof(T)>>(x)
				)
			);
		}
	}
};
