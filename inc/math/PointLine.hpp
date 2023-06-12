#pragma once
#include "math/Vertex.hpp"
#include <optional>
#include <algorithm>

namespace sm {
	
template<typename T, std::size_t N>
struct Segment;

template<typename T, std::size_t N>
struct Line {
	Vertex<T, N> a;
	Vertex<T, N> b;
	Vertex<T, N> direction;
	
	constexpr Line(Vertex<T,N> const& a, Vertex<T,N> const& b) noexcept
		: a(a)
		, b(b)
		, direction(b - a)
	{}
	
	constexpr auto at(T const& lambda) const noexcept{
		return a + lambda * direction;
	}
	
	constexpr T project(Vertex<T,N> const& point) const noexcept {
		return at(projection_lambda(point));
	}
	
	constexpr T projection_lambda(Vertex<T,N> const& point) const noexcept {
		return (direction * (point - a)) / (direction * direction);
	}
	
	constexpr Segment<T,N> project(Segment<T,N> const& segment) const noexcept;
};

/* does not work as intended...
 * PointLine pl{Line{a,b},c}; // Error: ‘auto’ parameter not permitted in this context
 */
template<typename T, std::size_t N>
Line(Vertex<T,N> const&, Vertex<T,N> const&)
	-> Line<T, N>
;

template<typename T, std::size_t N>
constexpr auto make_line(Vertex<T,N> const& a, Vertex<T,N> const& b) {
	return Line<T,N>{a,b};
}

template<typename T, std::size_t N>
struct Segment
	: Line<T, N>
{
	using Line<T,N>::Line;
	
	/** If other projected on *this is outside *this, {} is returned.
	 *  otherwise the projection clamped to *this is returned.
	 */
	constexpr std::optional<Segment> project(Segment const& other, double eps) const noexcept {
		double lambda_other_start = this->projection_lambda(other.a);
		double lambda_other_end   = this->projection_lambda(other.b);
		if(    (lambda_other_start < 0 - eps && lambda_other_end < 0 - eps)
			|| (lambda_other_start > 1 + eps && lambda_other_end > 1 + eps)
		) {
			return {};
		}
		return Segment{
			  this->at(std::clamp(lambda_other_start, 0.0 - eps, 1.0 + eps))
			, this->at(std::clamp(lambda_other_end,   0.0 - eps, 1.0 + eps))
		};
	}
};

template<typename T, std::size_t N>
Segment(Vertex<T,N> const&, Vertex<T,N> const&)
	-> Segment<T, N>
;

template<typename T, std::size_t N>
constexpr auto make_segment(Vertex<T,N> const& a, Vertex<T,N> const& b) {
	return Segment<T,N>{a,b};
}

template<typename T, std::size_t N>
constexpr Segment<T,N> Line<T,N>::project(Segment<T,N> const& segment) const noexcept {
	return {project(segment.a), project(segment.b)};
}

template<typename T, std::size_t N>
struct PointLine {
	Line<T, N>   line;
	Vertex<T, N> point;
	T            lambda;
	
	constexpr PointLine(Line<T, N>const& line, Vertex<T, N> const& point) noexcept
		: line(line)
		, point(point)
		, lambda(line.projection_lambda(point))
	{}

	constexpr Vertex<T,N> footpoint() const  noexcept {
		return line.at(lambda);
	}
	constexpr T distance() const noexcept {
		return delta().length();
	}
	constexpr Vertex<T,N> delta() const noexcept {
		return point - footpoint();
	}
};

template<typename T, std::size_t N>
PointLine(Line<T,N> const&, Vertex<T,N> const&)
	-> PointLine<T, N>
;

template<typename T, std::size_t N>
struct PointSegment {
	Segment<T, N> segment;
	Vertex<T, N>  point;
	T             lambda;
	
	constexpr PointSegment(Segment<T, N>const& segment, Vertex<T, N> const& point) noexcept
		: segment(segment)
		, point(point)
		, lambda(segment.projection_lambda(point))
	{}

	constexpr Vertex<T,N> footpoint() const  noexcept {
		return segment.at(lambda);
	}
	constexpr T distance() const noexcept {
		return delta().length();
	}
	constexpr Vertex<T,N> delta() const noexcept {
		if(lambda < 0) {
			return point - segment.a;
		}
		if(lambda > 1) {
			return point - segment.b;
		}
		return point - footpoint();
	}
	constexpr bool is_footpoint_valid() const noexcept {
		return (lambda >= 0) && (lambda <= 1);
	}
};

template<typename T, std::size_t N>
PointSegment(Line<T,N> const&, Vertex<T,N> const&)
	-> PointSegment<T, N>
;

} /** namespace sm */
