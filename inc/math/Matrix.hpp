#pragma once
#include "Vertex.hpp"
#include <optional>

template<detail::numeric T, std::size_t N, std::size_t M>
struct Matrix
	: std::array<Vertex<T, M>, N>
{
	using underlying_t = T;

	constexpr static std::size_t rows() {
		return N;
	}
	constexpr static std::size_t cols() {
		return M;
	}
	friend
	constexpr auto operator+=(Matrix& a, Matrix const& b) noexcept
		-> Matrix&
	{
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((a[Is] += b[Is]), ...);
		}(std::make_index_sequence<rows()>{});
		return a;
	}
	friend
	constexpr auto operator-=(Matrix& a, Matrix const& b) noexcept
		-> Matrix&
	{
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((a[Is] -= b[Is]), ...);
		}(std::make_index_sequence<rows()>{});
		return a;
	}
	friend
	constexpr auto operator*=(Matrix& m, T const& x) noexcept
		-> Matrix&
	{
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((m[Is] *= x), ...);
		}(std::make_index_sequence<rows()>{});
		return m;
	}
	friend
	constexpr auto operator/=(Matrix& m, T const& x) noexcept
		-> Matrix&
	{
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((m[Is] /= x), ...);
		}(std::make_index_sequence<rows()>{});
		return m;
	}
	friend
	constexpr auto operator-(Matrix const& m) noexcept
		-> Matrix
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return Matrix{-m[Is]...};
		}(std::make_index_sequence<rows()>{});
	}
	friend
	constexpr auto operator+(Matrix const& a, Matrix const& b) noexcept
		-> Matrix
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return Matrix{a[Is] + b[Is]...};
		}(std::make_index_sequence<rows()>{});
	}
	friend
	constexpr auto operator-(Matrix const& a, Matrix const& b) noexcept
		-> Matrix
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return Matrix{a[Is] - b[Is]...};
		}(std::make_index_sequence<rows()>{});
	}
	friend
	constexpr auto operator*(Matrix const& m, T const& x) noexcept
		-> Matrix
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return Matrix{m[Is] * x...};
		}(std::make_index_sequence<rows()>{});
	}
	friend
	constexpr auto operator*(T const& x, Matrix const& m) noexcept
		-> Matrix
	{
		return m * x;
	}
	friend
	constexpr auto operator/(Matrix const& m, T const& x) noexcept
		-> Matrix
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return Matrix{m[Is] / x...};
		}(std::make_index_sequence<rows()>{});
	}
	friend
	constexpr auto operator*(Matrix const& m, Vertex<T, cols()> const& v) noexcept
		-> Vertex<T, rows()>
	{
		return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return Vertex<T, rows()>{m[Is] * v...};
		}(std::make_index_sequence<rows()>{});
	}
	template<std::size_t b_cols>
	friend
	constexpr auto operator*(Matrix const& a, Matrix<T, cols(), b_cols> const& b) noexcept
		-> Matrix<T, rows(), b_cols>
	{
		auto bT = b.transpose();
		auto result_row = [&]<std::size_t R>(
			std::integral_constant<std::size_t, R> row
		) {
			return [&]<std::size_t... Cs>(std::index_sequence<Cs...>) {
				return Vertex<T, b_cols>{
					a[R] * bT[Cs] ...
				};
			}(std::make_index_sequence<b_cols>{});
		};
		return [&]<std::size_t... Rs>(std::index_sequence<Rs...>) {
			return Matrix<T, rows(), b_cols>{
				result_row(std::integral_constant<std::size_t, Rs>{})...
			};
		}(std::make_index_sequence<rows()>{});
	}

	constexpr auto transpose() const noexcept
		-> Matrix<T, cols(), rows()>
	{
		auto result_row = [&]<std::size_t R>(
			std::integral_constant<std::size_t, R> row
		) {
			return [&]<std::size_t... Cs>(std::index_sequence<Cs...>) {
				return Vertex<T, rows()>{
					(*this)[Cs][R]...
				};
			}(std::make_index_sequence<rows()>{});
		};
		return [&]<std::size_t... Rs>(std::index_sequence<Rs...>) {
			return Matrix<T, cols(), rows()>{
				result_row(std::integral_constant<std::size_t, Rs>{})...
			};
		}(std::make_index_sequence<cols()>{});
	}

	template<typename OS>
	friend
	auto operator<<(OS& os, Matrix const& m)
		-> OS&
	{
		os << "[\n";
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			((os << "    " << m[Is] << '\n'), ...);
		}(std::make_index_sequence<rows()>{});
		os << "]\n";
		return os;
	}

	auto max() const noexcept
		-> T
	{
		T result{(*this)[0].max()};
		[&]<std::size_t... Is>(std::index_sequence<0, Is...>) {
			((result = (*this)[Is].max()), ...);
		}(std::make_index_sequence<rows()>{});
		return result;
	}
	auto min() const noexcept
		-> T
	{
		T result{(*this)[0].min()};
		[&]<std::size_t... Is>(std::index_sequence<0, Is...>) {
			((result = (*this)[Is].min()), ...);
		}(std::make_index_sequence<rows()>{});
		return result;
	}
	auto abs_max() const noexcept
		-> T
	{
		T result{(*this)[0].abs_max()};
		[&]<std::size_t... Is>(std::index_sequence<0, Is...>) {
			((result = (*this)[Is].abs_max()), ...);
		}(std::make_index_sequence<rows()>{});
		return result;
	}
};

template<typename Matrix>
inline constexpr bool is_matrix_v = false;

template<detail::numeric T,std::size_t Rows, std::size_t Cols>
inline constexpr bool is_matrix_v<Matrix<T, Rows, Cols>> = true;

template<typename T>
concept matrix = is_matrix_v<T>;

template<detail::numeric T, std::size_t N>
constexpr auto diagonal(Vertex<T,N> const& d) noexcept
	-> Matrix<T, N, N>
{
	auto result_at = [&]<std::size_t row, std::size_t col>(
		  std::integral_constant<std::size_t, row>
		, std::integral_constant<std::size_t, col>
	) {
		if constexpr(row == col) {
			return d[row];
		} else {
			return T{};
		}
	};
	auto result_row = [&]<std::size_t R>(
		std::integral_constant<std::size_t, R> row
	) {
		return [&]<std::size_t... Cs>(std::index_sequence<Cs...>) {
			return Vertex<T, N>{
				result_at(
					  std::integral_constant<std::size_t, R>{}
					, std::integral_constant<std::size_t, Cs>{}
				)...
			};
		}(std::make_index_sequence<N>{});
	};
	return [&]<std::size_t... Rs>(std::index_sequence<Rs...>) {
		return Matrix<T, N, N>{
			result_row(std::integral_constant<std::size_t, Rs>{})...
		};
	}(std::make_index_sequence<N>{});
}

template<detail::numeric T, std::size_t N>
constexpr auto identity() noexcept
	-> Matrix<T, N, N>
{
	return diagonal(
		[&]<std::size_t... Is>(std::index_sequence<Is...>) {
			return Vertex<T, N>{
				(static_cast<void>(Is), 1.0)...
			};
		}(std::make_index_sequence<N>{})
	);
}

namespace detail {
	template<std::size_t I>
	using index_constant = std::integral_constant<std::size_t, I>;


	template<detail::numeric T, std::size_t N, typename X>
		requires (matrix<X> && X::rows() == N)
			|| (vertex<X> && X::size() == N)
	constexpr void solve(Matrix<T, N, N>& A, X& x) {
		auto do_pivot = [&]<std::size_t R>(index_constant<R>) {
			auto max = std::abs(A[R][R]);
			std::size_t row_max = R;

			auto process_row = [&]<std::size_t I>(index_constant<I>) {
				auto abs = std::abs(A[I][R]);
				if(abs > max) {
					max     = abs;
					row_max = I;
				}
			};
			[&]<std::size_t... Is>(std::index_sequence<Is...>) {
				(process_row(index_constant<Is + R + 1>{}), ...);
			}(std::make_index_sequence<N - R - 1>{});

			if(row_max != R) {
				std::swap(A[R], A[row_max]);
				std::swap(x[R], x[row_max]);
			}
		};
		auto make_triangular = [&]() {
			auto process_sub_row = [&]<std::size_t R, std::size_t I>(index_constant<R>, index_constant<I>) {
				T Ai  = A[I][R];
				x[I] -= Ai * x[R];
				A[I] -= Ai * A[R];
			};
			auto process_row = [&]<std::size_t R>(index_constant<R> row) {
				do_pivot(row);
				T Arr = A[R][R];
				x[R] /= Arr;
				A[R] /= Arr;
				[&]<std::size_t... Is>(std::index_sequence<Is...>) {
					(process_sub_row(row, index_constant<R + 1 + Is>{}), ...);
				}(std::make_index_sequence<N - R - 1>{});
			};
			[&]<std::size_t... Rows>(std::index_sequence<Rows...>) {
				(process_row(index_constant<Rows>{}), ...);
			}(std::make_index_sequence<N>{});
		};
		auto diagonalize = [&]() {
			auto process_sub_row = [&]<std::size_t R, std::size_t I>(index_constant<R>, index_constant<I>) {
				x[I] -= A[I][R] * x[R];
				A[I] -= A[I][R] * A[R];
			};
			auto process_row = [&]<std::size_t R>(index_constant<R> row) {
				[&]<std::size_t... Rows>(std::index_sequence<Rows...>) {
					(process_sub_row(row, index_constant<Rows>{}), ...);
				}(std::make_index_sequence<R>{});
			};
			[&]<std::size_t... Is>(std::index_sequence<Is...>) {
				(process_row(index_constant<N - Is - 1>{}), ...);
			}(std::make_index_sequence<N>{});
		};
		make_triangular();
		diagonalize();
	}
} /* namespace detail */

template<detail::numeric T, std::size_t N>
constexpr auto invert(Matrix<T, N, N> A) noexcept
	-> Matrix<T, N, N>
{
	Matrix<T, N, N> y = identity<T, N>();
	detail::solve(A, y);
	return y;
}
template<detail::numeric T, std::size_t N>
constexpr auto solve(Matrix<T, N, N> A, Vertex<T, N> y) noexcept
	-> Vertex<T, N>
{
	detail::solve(A, y);
	return y;
}

template<detail::numeric T>
constexpr auto determinant(Matrix<T, 2, 2> const& A) noexcept
	-> T
{
	return A[0][0] * A[1][1] - A[0][1] * A[1][0];
}

template<detail::numeric T>
constexpr auto determinant(Matrix<T, 3, 3> const& A) noexcept
	-> T
{
	return
		  A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1])
		+ A[0][1] * (A[1][2] * A[2][0] - A[1][0] * A[2][2])
		+ A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0])
	;
}

template<detail::numeric T>
constexpr auto simple_inverse_orthonormal(
	  Matrix<T, 2, 2> const& A
) noexcept
	-> Matrix<T, 2, 2>
{
	return Matrix<T, 2, 2>{
		  Vertex<T, 2>{ A[1][1], -A[0][1]}
		, Vertex<T, 2>{-A[1][0],  A[0][0]}
	};
}

template<detail::numeric T>
constexpr auto simple_inverse(
	  Matrix<T, 2, 2> const& A
	, T const&               det_abs_min
) noexcept
	-> std::optional<Matrix<T, 2, 2>>
{
	T det = determinant(A);
	if(std::abs(det) < det_abs_min) {
		return {};
	}
	return simple_inverse_orthonormal(A) / det;
}

template<detail::numeric T>
constexpr auto simple_inverse_orthonormal(
	  std::integral_constant<std::size_t, 0>
	, Matrix<T, 3, 3> const& A
) noexcept
	-> Vertex<T, 3>
{
	return Vertex<T, 3>{
		  (A[1][1] * A[2][2] - A[1][2] * A[2][1])
		, (A[0][2] * A[2][1] - A[0][1] * A[2][2])
		, (A[0][1] * A[1][2] - A[0][2] * A[1][1])
	};
}
template<detail::numeric T>
constexpr auto simple_inverse_orthonormal(
	  std::integral_constant<std::size_t, 1>
	, Matrix<T, 3, 3> const& A
) noexcept
	-> Vertex<T, 3>
{
	return Vertex<T, 3>{
		  (A[1][2] * A[2][0] - A[1][0] * A[2][2])
		, (A[0][0] * A[2][2] - A[0][2] * A[2][0])
		, (A[0][2] * A[1][0] - A[0][0] * A[1][2])
	};
}
template<detail::numeric T>
constexpr auto simple_inverse_orthonormal(
	  std::integral_constant<std::size_t, 2>
	, Matrix<T, 3, 3> const& A
) noexcept
	-> Vertex<T, 3>
{
	return Vertex<T, 3>{
		  (A[1][0] * A[2][1] - A[1][1] * A[2][0])
		, (A[0][1] * A[2][0] - A[0][0] * A[2][1])
		, (A[0][0] * A[1][1] - A[0][1] * A[1][0])
	};
}

template<detail::numeric T>
constexpr auto simple_inverse_orthonormal(
	  Matrix<T, 3, 3> const& A
) noexcept
	-> Matrix<T, 3, 3>
{
	return Matrix<T, 3, 3>{
		  simple_inverse_orthonormal(std::integral_constant<std::size_t, 0>{}, A)
		, simple_inverse_orthonormal(std::integral_constant<std::size_t, 1>{}, A)
		, simple_inverse_orthonormal(std::integral_constant<std::size_t, 2>{}, A)
	};
}

template<detail::numeric T>
constexpr auto simple_inverse(
	  Matrix<T, 3, 3> const& A
	, T const&               det_abs_min
) noexcept
	-> std::optional<Matrix<T, 3, 3>>
{
	T det = determinant(A);
	if(std::abs(det) < det_abs_min) {
		return {};
	}
	return simple_inverse_orthonormal(A) / det;
}
