#pragma once
#include <bit>
#include "serializer/NativePodBackend.hpp"
#include "serializer/EndianSwappingPodBackend.hpp"

template<typename T>
struct DefaultPodBackend
	: std::conditional_t<
		  std::endian::native == std::endian::little
		, NativePodBackend<T>
		, EndianSwappingPodBackend<T>
	>
{};
