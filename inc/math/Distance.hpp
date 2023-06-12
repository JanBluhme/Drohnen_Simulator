#pragma once
#include "math/Vertex.hpp"

template<typename T>
struct Distance {
	T value;

	template<typename A, typename B, std::size_t DIM>
	constexpr static T make_value(Vertex<A,DIM> const& a, Vertex<B,DIM> const& b) noexcept {
		auto d = b - a;
		return d*d;
	}

	template<typename A, typename B, std::size_t DIM>
	constexpr Distance(Vertex<A,DIM> const& a, Vertex<B,DIM> const& b) noexcept
		: value(make_value(a,b))
	{}

	template<typename A>
	constexpr Distance(A const& value) noexcept
		: value(value * value)
	{}

	template<typename U>
	friend
	constexpr bool operator<(Distance const& a, Distance<U> const& b) noexcept {
		return a.value < b.value;
	}
	template<typename U>
	friend
	constexpr bool operator>(Distance const& a, Distance<U> const& b) noexcept {
		return b < a;
	}
	template<typename U>
	friend
	constexpr bool operator<=(Distance const& a, Distance<U> const& b) noexcept {
		return !(a > b);
	}
	template<typename U>
	friend
	constexpr bool operator>=(Distance const& a, Distance<U> const& b) noexcept {
		return !(a < b);
	}
	template<typename U>
	friend
	constexpr bool operator==(Distance const& a, Distance<U> const& b) noexcept {
		return a == Distance<decltype(b*b)>{b};
	}
	template<typename U>
	friend
	constexpr bool operator!=(Distance const& a, Distance<U> const& b) noexcept {
		return !(a == b);
	}

};

template<typename T, typename U, std::size_t DIM>
Distance(Vertex<T,DIM> const& a, Vertex<U,DIM> const& b)
	-> Distance<decltype((b - a) * (b - a))>
;
template<typename T>
Distance(T const& v)
	-> Distance<T>
;
