#pragma once
#include <concepts>
#include <cmath>
#include <array>
#include <utility>
#include <cassert>

namespace detail {
	template<typename T>
	concept numeric
		=  std::integral<T>
		|| std::floating_point<T>
	;
} /* namespace detail */

template<detail::numeric T, std::size_t N>
struct Vertex : std::array<T, N> {
	using underlying_t = T;

	friend
	constexpr auto operator+=(Vertex& a, Vertex const& b) noexcept
		-> Vertex&
	{
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((a[Is] += b[Is]), ...);
		}(std::make_index_sequence<N>{});
		return a;
	}

	friend
	constexpr auto operator-=(Vertex& a, Vertex const& b) noexcept
		-> Vertex&
	{
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((a[Is] -= b[Is]), ...);
		}(std::make_index_sequence<N>{});
		return a;
	}

	friend
	constexpr auto operator*=(Vertex& v, T const& x) noexcept
		-> Vertex&
	{
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((v[Is] *= x), ...);
		}(std::make_index_sequence<N>{});
		return v;
	}

	friend
	constexpr auto operator/=(Vertex& v, T const& x) noexcept
		-> Vertex&
	{
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((v[Is] /= x), ...);
		}(std::make_index_sequence<N>{});
		return v;
	}

	friend
	constexpr auto operator-(Vertex const& v) noexcept
		-> Vertex
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return Vertex{-v[Is]...};
		}(std::make_index_sequence<N>{});
	}

	friend
	constexpr auto operator+(Vertex const& a, Vertex const& b) noexcept
		-> Vertex
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return Vertex{a[Is] + b[Is]...};
		}(std::make_index_sequence<N>{});
	}

	friend
	constexpr auto operator-(Vertex const& a, Vertex const& b) noexcept
		-> Vertex
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return Vertex{a[Is] - b[Is]...};
		}(std::make_index_sequence<N>{});
	}

	friend
	constexpr auto operator*(Vertex const& v, T const& x)
		-> Vertex
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) noexcept {
			return Vertex{v[Is] * x...};
		}(std::make_index_sequence<N>{});
	}

	friend
	constexpr auto operator*(T const& x, Vertex const& v) noexcept
		-> Vertex
	{
		return v * x;
	}

	friend
	constexpr auto operator/(Vertex const& v, T const& x) noexcept
		-> Vertex
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return Vertex{v[Is] / x...};
		}(std::make_index_sequence<N>{});
	}

	friend
	constexpr auto operator*(Vertex const& a, Vertex const& b) noexcept
		-> T
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return ((a[Is] * b[Is]) + ...);
		}(std::make_index_sequence<N>{});
	}

	template<typename OS>
	friend
	auto operator<<(OS& os, Vertex const& v) noexcept
		-> OS&
	{
		os << '[';
		if constexpr(N) {
			os << v[0];
			[&]<std::size_t... Is>(std::index_sequence<0, Is...>) {
				((os << ", " << v[Is]), ...);
			}(std::make_index_sequence<N>{});
		}
		os << ']';
		return os;
	}

	auto max() const noexcept
		-> T
	{
		T result{(*this)[0]};
		[&]<std::size_t... Is>(std::index_sequence<0, Is...>) {
			((result = std::max(result, (*this)[Is])), ...);
		}(std::make_index_sequence<N>{});
		return result;
	}

	auto min() const noexcept
		-> T
	{
		T result{(*this)[0]};
		[&]<std::size_t... Is>(std::index_sequence<0, Is...>) {
			((result = std::min(result, (*this)[Is])), ...);
		}(std::make_index_sequence<N>{});
		return result;
	}

	auto abs_max() const noexcept
		-> T
	{
		T result{std::abs((*this)[0])};
		[&]<std::size_t... Is>(std::index_sequence<0, Is...>) {
			((result = std::max(result, std::abs((*this)[Is]))), ...);
		}(std::make_index_sequence<N>{});
		return result;
	}

	template<typename TT>
	constexpr auto as_vertex_of() const noexcept
		-> Vertex<TT, N>
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return Vertex<TT, N> {
				static_cast<TT>(this->operator[](Is))...
			};
		}(std::make_index_sequence<N>{});
	}

	static constexpr auto size() noexcept
		-> std::size_t
	{
		return N;
	}

	auto length() const noexcept
		-> T
	{
		return std::sqrt((*this) * (*this));
	}

	auto normalize() const noexcept
		-> Vertex
	{
		return (*this) / length();
	}

	constexpr auto is_finite() const noexcept
		-> bool
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return (std::isfinite(this->operator[](Is)) && ...);
		}(std::make_index_sequence<N>{});
	}

	template<typename Serializer, typename Buffer>
	static auto serialize(Buffer& buffer, Vertex const& v)
		-> bool
	{
		return Serializer::template serialize<Buffer,std::array<T, N>>(buffer, v);
	}

	template<typename Serializer, typename Buffer>
	static auto deserialize(Buffer& buffer, Vertex& v)
		-> bool
	{
		return Serializer::template deserialize<Buffer,std::array<T, N>>(buffer, v);
	}
};

template<typename Vertex>
inline constexpr bool is_vertex_v = false;

template<detail::numeric T,std::size_t N>
inline constexpr bool is_vertex_v<Vertex<T, N>> = true;

template<typename T>
concept vertex = is_vertex_v<T>;

template<detail::numeric T>
auto rotate(Vertex<T,2> const& v, T const& phi) noexcept
	-> Vertex<T,2>
{
	return Vertex<T, 2>{
		  Vertex<T, 2>{std::cos(phi), -std::sin(phi) } * v
		, Vertex<T, 2>{std::sin(phi),  std::cos(phi) } * v
	};
}

template<detail::numeric T>
constexpr auto ortho(Vertex<T,2> const& v) noexcept
	-> Vertex<T, 2>
{
	return Vertex<T,2>{-v[1], v[0]};
}

template<detail::numeric T>
constexpr auto ortho_neg(Vertex<T,2> const& v) noexcept
	-> Vertex<T, 2>
{
	return Vertex<T,2>{v[1], -v[0]};
}

template<detail::numeric T>
auto polar(T const& length, T const& phi) noexcept
	-> Vertex<T, 2>
{
	return Vertex<T, 2>{
		  length*std::cos(phi)
		, length*std::sin(phi)
	};
}

template<detail::numeric T>
auto orientation(Vertex<T,2> const& v) noexcept
	-> T
{
	return std::atan2(v[1], v[0]);
}

template<detail::numeric T>
	requires (!std::is_unsigned_v<T>)
constexpr auto cross_product(Vertex<T, 3> const& a, Vertex<T, 3> const& b) noexcept
	-> Vertex<T, 3>
{
	return {
		  a[1] * b[2] - a[2] * b[1]
		, a[2] * b[0] - a[0] * b[2]
		, a[0] * b[1] - a[1] * b[0]
	};
}

template<detail::numeric T>
	requires (!std::is_unsigned_v<T>)
constexpr auto cross_product(Vertex<T,2> const& a, Vertex<T,2> const& b) noexcept
	-> T
{
	return a[0] * b[1] - a[1] * b[0];
}
