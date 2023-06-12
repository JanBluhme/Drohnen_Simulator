#pragma once
#include "util/EndianSwapper.hpp"
#include "serializer/NativePodBackend.hpp"

template<typename T>
struct EndianSwappingPodBackend {
	template<typename Buffer>
	static bool serialize(Buffer& buffer, T v) {
		static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
		v = EndianSwapper::byte_swap(v);
		return buffer.insert(&v, sizeof(v));
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, T& v) {
		static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
		bool result = buffer.extract(&v, sizeof(v));
		v = EndianSwapper::byte_swap(v);
		return result;
	}
	template<typename Buffer>
	static bool serialize_range(Buffer& buffer, T const* begin, T const* end) {
		return NativePodBackend_range<T, EndianSwappingPodBackend>::serialize_range(buffer, begin, end);
	}
	template<typename Buffer>
	static bool deserialize_range(Buffer& buffer, T* begin, T* end) {
		return NativePodBackend_range<T, EndianSwappingPodBackend>::deserialize_range(buffer, begin, end);
	}
};
