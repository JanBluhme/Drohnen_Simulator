#pragma once
#include "math/Vertex.hpp"
#include "math/Matrix.hpp"
#include "math/r3/line_plane_intersection_lambdas.hpp"
#include "util/iterator_of.hpp"

struct Ray3 {
	Vertex<double, 3> point;
	Vertex<double, 3> direction;

	auto at(double lambda) const noexcept
		-> Vertex<double, 3>
	{
		return point + direction * lambda;
	}
};

struct Line3 {
	Vertex<double, 3> point;
	Vertex<double, 3> direction;

	auto at(double lambda) const noexcept
		-> Vertex<double, 3>
	{
		return point + direction * lambda;
	}
};
struct Segment3 {
	Vertex<double, 3> point;
	Vertex<double, 3> direction;

	auto at(double lambda) const noexcept
		-> Vertex<double, 3>
	{
		return point + direction * lambda;
	}
};

struct Triangle {
	using V3 = Vertex<double, 3>;
	V3 A;
	V3 B;
	V3 C;
};

class TriangleIntersector {
	using V3 = Triangle::V3;
	Triangle T;

	V3 AB = T.B - T.A;
	V3 AC = T.C - T.A;
public:
	TriangleIntersector(Triangle const& T)
		: T{T}
	{}

	constexpr auto triangle() const noexcept
		-> Triangle const&
	{
		return T;
	}
	// may return ray lambda
	// lambda[0] -> ray
	// lambda[1] -> AB
	// lambda[2] -> AC
	constexpr auto get_intersection_lambdas(V3 const& point, V3 const& direction) const noexcept
		-> std::optional<Vertex<double, 3>>
	{
		return line_plane_intersection_lambdas(
			point, direction, T.A, AB, AC, 1.0e-8
		);
	};

	static auto are_intersection_lambdas_inside(
		Vertex<double, 3> const& lambdas
	) noexcept
		-> bool
	{
		if(    lambdas[1] >= 0.0 && lambdas[1] <= 1.0
			&& lambdas[2] >= 0.0 && lambdas[2] + lambdas[1] <= 1.0
		) {
			return true;
		}
		return false;
	}

	static auto check_intersection_by_lambdas(
		std::optional<Vertex<double, 3>> const& o_lambdas
	) noexcept
		-> bool
	{
		if(o_lambdas) {
			return are_intersection_lambdas_inside(*o_lambdas);
		}
		return false;
	}
	// may return ray lambda
	auto intersected_by(Line3 const& line) const
		-> std::optional<double>
	{
		auto o_lambdas = get_intersection_lambdas(line.point, line.direction);
		if(check_intersection_by_lambdas(o_lambdas)) {
			return (*o_lambdas)[0];
		}
		return {};
	}
	// may return ray lambda
	auto intersected_by(Ray3 const& ray) const
		-> std::optional<double>
	{
		auto o_lambdas = get_intersection_lambdas(ray.point, ray.direction);
		if(check_intersection_by_lambdas(o_lambdas)
			&& (*o_lambdas)[0] >= 0.0
		) {
			return (*o_lambdas)[0];
		}
		return {};
	}
	// may return ray lambda
	auto intersected_by(Segment3 const& segment) const
		-> std::optional<double>
	{
		auto o_lambdas = get_intersection_lambdas(segment.point, segment.direction);
		if(check_intersection_by_lambdas(o_lambdas)
			&& (*o_lambdas)[0] >= 0.0
			&& (*o_lambdas)[0] <= 1.0
		) {
			return (*o_lambdas)[0];
		}
		return {};
	}
	template<iterator_of<TriangleIntersector> Iterator>
	friend
	auto any_intersection(Iterator first, Iterator last, Segment3 const& segment)
		-> bool
	{
		while(first != last) {
			if(first->intersected_by(segment)) {
				return true;
			}
			++first;
		}
		return false;
	}
	template<iterator_of<TriangleIntersector> Iterator>
	friend
	auto first_intersection(Iterator first, Iterator last, Ray3 const& ray, double lambda_ray_min)
		-> std::pair<bool, double>
	{
		std::pair<bool, double> result{false, lambda_ray_min};
		constexpr double max = std::numeric_limits<double>::max();
		for(; first != last; ++first) {
			TriangleIntersector const& T = *first;
			Matrix<double, 3, 3> A {
				  V3{-ray.direction[0], T.AB[0], T.AC[0]}
				, V3{-ray.direction[1], T.AB[1], T.AC[1]}
				, V3{-ray.direction[2], T.AB[2], T.AC[2]}
			};
			double det = determinant(A);
			if(std::abs(det) > 1e-6) {
				V3 y  = ray.point  - T.T.A;
				V3 Ai0 = simple_inverse_orthonormal(std::integral_constant<std::size_t, 0>{}, A);
				double lambda_ray = (Ai0 * y) / det;
				if(lambda_ray > 0.0 && lambda_ray < result.second) {
					V3 Ai1 = simple_inverse_orthonormal(std::integral_constant<std::size_t, 1>{}, A);
					double lambda_AB = (Ai1 * y) / det;
					if(lambda_AB >= 0.0 && lambda_AB <= 1.0) {
						V3 Ai2 = simple_inverse_orthonormal(std::integral_constant<std::size_t, 2>{}, A);
						double lambda_AC = (Ai2 * y) / det;
						if(lambda_AC >= 0.0 && lambda_AB + lambda_AC <= 1.0) {
							result.first = true;
							result.second = lambda_ray;
						}
					}
				}
			}
		}
		return result;
	}
};
