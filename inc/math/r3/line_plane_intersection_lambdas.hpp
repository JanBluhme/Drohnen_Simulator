#pragma once
#include "math/Matrix.hpp"
#include <optional>

template<detail::numeric T>
constexpr auto line_plane_intersection_lambdas(
	  Vertex<T, 3> const& line_point
	, Vertex<T, 3> const& line_direction
	, Vertex<T, 3> const& plane_point
	, Vertex<T, 3> const& plane_direction0
	, Vertex<T, 3> const& plane_direction1
	, T const&            determinant_abs_min
) noexcept
	-> std::optional<Vertex<T, 3>>
{
	// line_point + line_direction * r  = plane_point                      + plane_direction0 * s + plane_direction1 * t
	// line_point                       = plane_point - line_direction * r + plane_direction0 * s + plane_direction1 * t
	// line_point - plane_point         =             - line_direction * r + plane_direction0 * s + plane_direction1 * t
	//
	// (line_point_x - plane_point_x)   (-line_direction_x * r + plane_direction0_x * s + plane_direction1_x * t)
	// (line_point_y - plane_point_y) = (-line_direction_y * r + plane_direction0_y * s + plane_direction1_y * t)
	// (line_point_z - plane_point_z)   (-line_direction_z * r + plane_direction0_z * s + plane_direction1_z * t)
	//
	// (line_point_x - plane_point_x)   (-line_direction_x, plane_direction0_x, plane_direction1_x)   (r)
	// (line_point_y - plane_point_y) = (-line_direction_y, plane_direction0_y, plane_direction1_y) * (s)
	// (line_point_z - plane_point_z)   (-line_direction_z, plane_direction0_z, plane_direction1_z)   (t)
	//
	//  ^-- y                    ^-- A                  ^-- x
	//
	// y        = A x
	// inv(A) y = x

	Matrix<T, 3, 3> A {
		  Vertex<T, 3>{-line_direction[0], plane_direction0[0], plane_direction1[0]}
		, Vertex<T, 3>{-line_direction[1], plane_direction0[1], plane_direction1[1]}
		, Vertex<T, 3>{-line_direction[2], plane_direction0[2], plane_direction1[2]}
	};
	auto o_Ai = simple_inverse(A, determinant_abs_min);
	if(!o_Ai) {
		return {};
	}
	Vertex<T, 3> y  = line_point  - plane_point;
	return *o_Ai * y;
}
