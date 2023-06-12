#pragma once
#include "math/Vertex.hpp"
#include "math/PointLine.hpp"
#include "math/Distance.hpp"
#include <algorithm>
#include <optional>
#include <variant>

namespace sm {
	
struct IntersectionHelper {
	double           det;
	Vertex<double,2> lambda;
	
	template<
		  typename thing_with_support_vector_and_direction_A
		, typename thing_with_support_vector_and_direction_B
	>
	constexpr IntersectionHelper(
		  thing_with_support_vector_and_direction_A const& a
		, thing_with_support_vector_and_direction_B const& b
	) noexcept
		: det(ortho(b.direction)*a.direction)
		, lambda{
			  ortho(b.direction) * (b.a - a.a) / (std::abs(det) > 1e-6 ? det : 1.0)
			, ortho(a.direction) * (b.a - a.a) / (std::abs(det) > 1e-6 ? det : 1.0)
		}
	{}
};
	
struct LineLineIntersection
	: IntersectionHelper
{
	Line<double,2> a;
	Line<double,2> b;
	
	constexpr LineLineIntersection(
		  Line<double,2> const& a
		, Line<double,2> const& b
	) noexcept
		: IntersectionHelper{a,b}
		, a{a}
		, b{b}
	{}
	constexpr Vertex<double,2> intersection() const noexcept {
		return a.at(lambda[0]);
	}
	constexpr bool is_collinear(double eps) const noexcept {
		return Distance(PointLine(a, b.a).footpoint(), b.a) < Distance(eps)
			&& Distance(PointLine(a, b.b).footpoint(), b.b) < Distance(eps)
		;
	}
};

struct SegmentSegmentIntersection
	: IntersectionHelper
{
	Segment<double,2> a;
	Segment<double,2> b;

	constexpr SegmentSegmentIntersection(
		  Segment<double,2> const& a
		, Segment<double,2> const& b
	) noexcept
		: IntersectionHelper{a,b}
		, a{a}
		, b{b}
	{}
	constexpr Vertex<double,2> intersection_point() const noexcept {
		return a.at(lambda[0]);
	}
	constexpr bool is_intersection_valid() const noexcept {
		return lambda[0] >= 0 && lambda[0] <= 1.0
			&& lambda[1] >= 0 && lambda[1] <= 1.0
		;
	}
	constexpr bool is_collinear(double eps) const noexcept{
		return Distance(PointSegment(a, b.a).footpoint(), b.a) < Distance(eps)
			&& Distance(PointSegment(a, b.b).footpoint(), b.b) < Distance(eps)
		;
	}
	constexpr std::optional<Vertex<double,2>> intersection() const noexcept {
		if(is_intersection_valid()) {
			return intersection_point();
		}
		return {};
	}
};

} /** namespace sm */
