#pragma once
#include <cstdint>
#include <type_traits>

template<typename T>
struct NativePodBackend;

template<typename T>
struct NativePodBackend_integral_or_floating_point {
	template<typename Buffer>
	static bool serialize(Buffer& buffer, T v) {
		static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
		return buffer.insert(&v, sizeof(v));
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, T& v) {
		static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
		return buffer.extract(&v, sizeof(v));
	}
};
template<typename T>
struct NativePodBackend_range_integral_or_floating_point {
	template<typename Buffer>
	static bool serialize_range(Buffer& buffer, T const* begin, T const* end) {
		static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
		return buffer.insert(begin, (end - begin) * sizeof(T));
	}
	template<typename Buffer>
	static bool deserialize_range(Buffer& buffer, T* begin, T* end) {
		static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
		return buffer.extract(begin, (end - begin) * sizeof(T));
	}
};
template<typename T, template<typename>class S>
struct NativePodBackend_enum {
	using SS = S<std::underlying_type_t<T>>;
	template<typename Buffer>
	static bool serialize(Buffer& buffer, T const& v) {
		return SS::serialize(buffer, static_cast<std::underlying_type_t<T>>(v));
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, T& v) {
		std::underlying_type_t<T> temp;
		if(SS::deserialize(buffer, temp)) {
			v = static_cast<T>(temp);
			return true;
		}
		return false;
	}
};
template<template<typename>class S>
struct NativePodBackend_bool {
	using SS = S<uint8_t>;
	template<typename Buffer>
	static bool serialize(Buffer& buffer, bool v) {
		uint8_t vv = v ? 1 : 0;
		return SS::serialize(buffer, vv);
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, bool& v) {
		uint8_t vv;
		if(!SS::deserialize(buffer, vv)) {
			return false;
		}
		v = vv == 1 ? true : false;
		return true;
	}
};
template<typename T, template<typename>class S>
struct NativePodBackend_range {
	using SS = S<T>;
	template<typename Buffer>
	static bool serialize_range(Buffer& buffer, T const* begin, T const* end) {
		auto reset_point = buffer.reset_point();
		while(begin != end) {
			if(!SS::serialize(buffer, *begin)) {
				reset_point.apply();
				return false;
			}
			++begin;
		}
		return true;
	}
	template<typename Buffer>
	static bool deserialize_range(Buffer& buffer, T* begin, T* end) {
		auto reset_point = buffer.reset_point();
		while(begin != end) {
			if(!SS::deserialize(buffer, *begin)) {
				reset_point.apply();
				return false;
			}
			++begin;
		}
		return true;
	}
};
template<typename T>
struct NativePodBackend
	: std::conditional_t<
		std::is_integral_v<T> || std::is_floating_point_v<T>
		, NativePodBackend_integral_or_floating_point<T>
		, std::conditional_t<
			std::is_enum_v<T>
			, NativePodBackend_enum<T, NativePodBackend>
			, NativePodBackend_bool<NativePodBackend>
		>
	>
	, std::conditional_t<
		std::is_integral_v<T> || std::is_floating_point_v<T>
		, NativePodBackend_range_integral_or_floating_point<T>
		, NativePodBackend_range<T, NativePodBackend>
	>
{};
