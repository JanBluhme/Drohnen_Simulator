#pragma once
#include "math/Vertex.hpp"
#include "math/LineLineIntersection.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include <iostream>

namespace sm {

struct ConvexPolygon {
	std::vector<Vertex<double,2>> points;
	
	ConvexPolygon() = default;

	ConvexPolygon(std::vector<Vertex<double,2>> set) {
		// https://www.tutorialspoint.com/cplusplus-program-to-implement-graham-scan-algorithm-to-find-the-convex-hull
		assert(set.size() > 2);
		auto move_min_to_front = [&]() {
			auto it = std::min_element(
				  set.begin()
				, set.end()
				, [](Vertex<double,2> const& a, Vertex<double,2> const& b) {
					if(a[1] < b[1]) { return true; }
					if(a[1] > b[1]) { return false;}
					return a[0] < b[0];
				}
			);
			std::swap(*set.begin(), *it);
		};
		enum class Rotation {
			  ANTI_CLOCKWISE
			, CLOCKWISE
			, COLINEAR
		};
		auto direction = [](Vertex<double,2> const& a, Vertex<double,2> const&  b, Vertex<double,2> const& c) {
			double cp = cross_product(c - b, b - a);
			if(cp < 0) {
				return Rotation::ANTI_CLOCKWISE;
			}
			if(cp > 0) {
				return Rotation::CLOCKWISE;
			}
			return Rotation::COLINEAR;
		};
		auto ascending_angle_sort = [&]() {
			std::sort(
				  set.begin() + 1
				, set.end()
				, [&](Vertex<double,2> const& a, Vertex<double,2> const& b) {
					Rotation dir = direction(set[0], a, b);
					if(dir == Rotation::COLINEAR) {
						Vertex<double,2> A = a - set[0];
						Vertex<double,2> B = b - set[0];
						return A*A > B*B;
					}
					return dir == Rotation::ANTI_CLOCKWISE;
				}
			);
			set.erase(
				std::unique(
					  set.begin() + 1
					, set.end()
					, [&](Vertex<double,2> const& a, Vertex<double,2> const& b) {
						return direction(set[0], a, b) == Rotation::COLINEAR;
					}
				)
				, set.end()
			);
		};
		
		auto scan = [&]() {
			std::vector<Vertex<double,2>>& stack = points;
			auto top = [&stack]()
				-> Vertex<double,2>&
			{
				return stack[stack.size() - 1];
			};
			auto secondTop = [&stack]()
				-> Vertex<double,2>&
			{
				return stack[stack.size() - 2];
			};
			for(std::size_t i = 0; i < set.size(); ++i) {
				while( stack.size() > 1
					&& direction(secondTop(), top(), set[i]) != Rotation::ANTI_CLOCKWISE
				) {
					// when top, second top and ith point are not making left turn, remove point
					stack.pop_back();
				}
				stack.push_back(set[i]);
			}
		};
		
		move_min_to_front();
		ascending_angle_sort();
		scan();
	}

	std::size_t size() const {
		return points.size();
	}
	Vertex<double,2>& operator[](int i) {
		if(i < 0) {
			return points[(size() + i) % size()];
		}
		return points[i % size()];
	}
	Vertex<double,2> const& operator[](int i) const {
		if(i < 0) {
			return points[(size() + i) % size()];
		}
		return points[i % size()];
	}
	Line<double,2> segment(int i) const {
		return {(*this)[i], (*this)[i + 1]};
	}
	
	bool contains(Vertex<double,2>const& point) const {
		for(std::size_t i = 0; i < size(); ++i) {
			Line<double,2> s = segment(i);
			if(cross_product(s.a - point, s.b - point) <= 0) {
				return false;
			}
		}
		return true;
	}
	
	bool is_intersected(Vertex<double,2>& enter, Vertex<double,2>& leave, Line<double,2> const& segment) const {
		for(std::size_t i = 0; i < size(); ++i) {
			if(segment.a == (*this)[i]) {
				for(std::size_t j = 0; j < size(); ++j) {
					if(    i != j
						&& (i + 1)%size() != j
						&& (j + 1)%size() != i
						&& segment.b == (*this)[j]
					) {
						enter = segment.a;
						leave = segment.b;
						return true;
					}
				}
			}
		}
		double lambda_E = 0.0;
		double lambda_A = 1.0;
		bool intersection_detected = false;
		for(std::size_t i = 0; i < size(); ++i) {
			LineLineIntersection line_line(segment, this->segment(i));
			if(std::abs(line_line.det) < 1e-6) {
				// parallel: no intersection with current segment;
				continue;
			}
			double eps = 1e-6;
			if(line_line.lambda[0] < eps || line_line.lambda[0] > 1 - eps) {
				// intersection not BETWEEN s1 and s2
				continue;
			}
			if(line_line.lambda[1] < eps || line_line.lambda[1] > 1 - eps) {
				// intersection not BETWEEN p_i, p_{i+1}
				continue;
			}
			intersection_detected = true;
			lambda_E = std::min(lambda_E, line_line.lambda[0]);
			lambda_A = std::max(lambda_A, line_line.lambda[0]);
		}
		if(intersection_detected) {
			enter = segment.at(lambda_E);
			leave = segment.at(lambda_A);
			return true;
		}
		return false;
	}
};

} /**namespace sm*/
