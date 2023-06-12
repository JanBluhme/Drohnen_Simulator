#pragma once
#include "math/Vertex.hpp"
#include "math/PointLine.hpp"

namespace sm {

struct Rectangle {
	Vertex<double,2> min;
	Vertex<double,2> max;
	
	constexpr Vertex<double,2> center() const noexcept {
		return 0.5 * (min + max);
	}
	constexpr double width() const noexcept{
		return max[0] - min[0];
	}
	constexpr double height() const {
		return max[1] - min[1];
	}
	
	constexpr Rectangle scale(double factor) const noexcept {
		Vertex<double,2> c = center();
		Vertex<double,2> d{
			  factor * width()/2
			, factor * height()/2
		};
		return Rectangle {c - d, c + d};
	}
	
	constexpr double contains(Vertex<double,2> const& point) const noexcept {
		return point[0] >= min[0]
			&& point[0] <= max[0]
			&& point[1] >= min[1]
			&& point[1] <= max[1]
		;
	}
	constexpr Vertex<double,2> const& A() const {
		return min;
	}
	constexpr Vertex<double,2> B() const {
		return Vertex<double,2>{max[0], min[1]};
	}
	constexpr Vertex<double,2> const& C() const {
		return max;
	}
	constexpr Vertex<double,2> D() const {
		return Vertex<double,2>{min[0], max[1]};
	}
	constexpr double min_distance(Vertex<double,2> const& point) const noexcept {
		if(contains(point)) {
			return 0;
		}
		double d =      PointSegment{{A(), B()}, point}.distance();
		d = std::min(d, PointSegment{{B(), C()}, point}.distance());
		d = std::min(d, PointSegment{{C(), D()}, point}.distance());
		d = std::min(d, PointSegment{{D(), A()}, point}.distance());
		return d;
	}
};

} /** namespace sm */
