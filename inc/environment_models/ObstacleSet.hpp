#pragma once
#include "robo_commands.hpp"
#include "math/r3/Triangle.hpp"
#include "math/r3/Transform.hpp"
#include "simple_gl/make_simple_cone.hpp"
#include "config/Robot.hpp"

template<typename MeshData>
auto make_intersectors(MeshData const& mesh_data)
		-> std::vector<TriangleIntersector>
{
	auto mesh_triangles = mesh_data.triangles();
	std::vector<TriangleIntersector> result;
	result.reserve(mesh_triangles.size());
	std::transform(
		  mesh_triangles.begin()
		, mesh_triangles.end()
		, std::back_inserter(result)
		, [](Triangle const& T) {
			return TriangleIntersector{T};
		}
	);
	return result;
}

struct ObstacleGeometry {
	using mesh_data_t = mesh_data_by_tags_t<has_normal_tag, has_tex_tag>;

	mesh_data_t                      mesh_data;
	BoundingBox                      bounding_box           = mesh_data.bounding_box();
	Vertex<double, 3>                bounding_box_center    = bounding_box.center();
	Vertex<double, 3>                bounding_box_size      = bounding_box.size();
	double                           bounding_sphere_radius = std::max({
		  bounding_box_size[0]
		, bounding_box_size[1]
		, bounding_box_size[2]
	}) / 2.0;
	mesh_data_t                      bounding_box_mesh_data = [&]() {
		return Transform::translate(bounding_box.center())
			* make_simple_cube<true, false>(
				bounding_box_size[0], bounding_box_size[1], bounding_box_size[2]
			)
		;
	}();
	std::vector<TriangleIntersector> triangles              = make_intersectors(mesh_data);
	std::vector<TriangleIntersector> triangles_bounding_box = make_intersectors(bounding_box_mesh_data);

	ObstacleGeometry(mesh_data_t const& mesh_data)
		: mesh_data{mesh_data}
	{}

	constexpr auto point_in_bounding_sphere(Vertex<double, 3> const& point) const noexcept
		-> bool
	{
		Vertex<double,3> d = point - bounding_box_center;
		return d * d <= bounding_sphere_radius * bounding_sphere_radius;
	}
	constexpr auto line_intersects_bounding_sphere(Vertex<double, 3> const& point, Vertex<double, 3> const& direction) const noexcept
		-> bool
	{
		double lambda = (direction * (bounding_box_center - point)) / (direction * direction);
		return point_in_bounding_sphere(point + lambda * direction);
	}
	auto intersect(Ray3 const& ray, Transform const& Ti, std::optional<double> lambda_min) const
		-> std::optional<double>
	{
		Ray3 test{Ti.position(ray.point), Ti.direction(ray.direction)};
		if(line_intersects_bounding_sphere(test.point, test.direction)) {
			double min = lambda_min
				? *lambda_min
				: std::numeric_limits<double>::max()
			;
			if(auto p = first_intersection(triangles_bounding_box.begin(), triangles_bounding_box.end(), test, min)
				; p.first
			) {
				auto pp = first_intersection(triangles.begin(), triangles.end(), test, min);
				if(pp.first) {
					return pp.second;
				}
			}
		}
		return lambda_min;
	}

	auto inside(Vertex<double,3> const& point, Transform const& Ti) const
		-> bool
	{
		Vertex<double,3> test{Ti.position(point)};
		if(    /*point_in_bounding_sphere(test)
			&& */bounding_box.contains(test)
		) {
			Segment3 seg{
				test, bounding_box_center - test
			};
			return !any_intersection(triangles.begin(), triangles.end(), seg);
		}
		return false;
	}
	auto intersects(Segment3 const& segment, Transform const& Ti) const
		-> bool
	{
		Segment3 test{Ti.position(segment.point), Ti.direction(segment.direction)};
		if(line_intersects_bounding_sphere(test.point, test.direction)) {
			if(any_intersection(triangles_bounding_box.begin(), triangles_bounding_box.end(), test)
				|| bounding_box.contains(test.point)
				|| bounding_box.contains(test.point + test.direction)
			) {
				Segment3 test_A{
					test.point, bounding_box_center - test.point
				};
				Segment3 test_B{
					test.point + test.direction, bounding_box_center - (test.point + test.direction)
				};
				return  any_intersection(triangles.begin(), triangles.end(), test)
					|| !any_intersection(triangles.begin(), triangles.end(), test_A)
					|| !any_intersection(triangles.begin(), triangles.end(), test_B)
				;
			}
		}
		return false;
	}
};

struct ObstacleSet {
	struct ObstacleClass {
		ObstacleGeometry const raw;
		ObstacleGeometry const grown;

		ObstacleClass(ObstacleGeometry const& raw, ObstacleGeometry const& grown)
			: raw{raw}
			, grown{grown}
		{}
	};
	struct ObstacleGeometryAccessor_Raw {
		constexpr static auto access(ObstacleClass const* obstacle_class) noexcept
			-> ObstacleGeometry const&
		{
			return obstacle_class->raw;
		}
	};
	struct ObstacleGeometryAccessor_Grown {
		constexpr static auto access(ObstacleClass const* obstacle_class) noexcept
			-> ObstacleGeometry const&
		{
			return obstacle_class->grown;
		}
	};

	using class_id_t  = ObstacleClass const*;
	using object_id_t = std::size_t;
	using exchange_t  = std::vector<std::pair<class_id_t, Transform>>;
	constexpr static std::size_t                      grow_stacks      = 2;
	constexpr static std::size_t                      grow_sectors     = 2;
	constexpr static std::size_t                      cylinder_sectors = 12;
	double                                            grow_radius;
	std::vector<std::unique_ptr<ObstacleClass const>> geometries;
	std::map<Vertex<double,3>, class_id_t>            know_boxes;
	std::map<Vertex<double,2>, class_id_t>            know_cylinders;
	std::vector<class_id_t>                           class_lookup;
	exchange_t                                        obstacles_inv;
	exchange_t                                        obstacles;

	template<typename Accessor>
	struct View {
		using accesor_t = Accessor;
		exchange_t const& obstacles_inv;

		constexpr auto access(ObstacleClass const* obstacle_class) const noexcept
			-> ObstacleGeometry const&
		{
			return accesor_t::access(obstacle_class);
		}

		auto intersect(Ray3 const& ray, std::optional<double> lambda_min = {}) const
			-> std::optional<double>
		{
			for(auto const& p : obstacles_inv) {
				lambda_min = access(p.first).intersect(ray, p.second, lambda_min);
			}
			return lambda_min;
		}
		auto intersects(Segment3 const& segment) const
			-> bool
		{
			for(auto const& p : obstacles_inv) {
				if(access(p.first).intersects(segment, p.second)) {
					return true;
				}
			}
			return false;
		}
		auto inside(Vertex<double,3> const& point) const
			-> bool
		{
			for(auto const& p : obstacles_inv) {
				if(access(p.first).inside(point, p.second)) {
					return true;
				}
			}
			return false;
		}
		template<typename T>
		auto intersect(Ray3 const& ray, T includes_object_id, std::optional<double> lambda_min = {}) const
			-> std::optional<double>
			requires std::is_invocable_r_v<bool, T, object_id_t>
		{
			for(std::size_t i = 0, last = obstacles_inv.size(); i < last; ++i) {
				if(includes_object_id(i)) {
					auto const& p = obstacles_inv[i];
					lambda_min = access(p.first).intersect(ray, p.second, lambda_min);
				}
			}
			return lambda_min;
		}

		template<typename T>
		auto intersects(Segment3 const& segment, T includes_object_id) const
			-> bool
			requires std::is_invocable_r_v<bool, T, object_id_t>
		{
			for(std::size_t i = 0, last = obstacles_inv.size(); i < last; ++i) {
				if(includes_object_id(i)) {
					auto const& p = obstacles_inv[i];
					if(access(p.first).intersects(segment, p.second)) {
						return true;
					}
				}
			}
			return false;
		}
		template<typename T>
		auto inside(Vertex<double,3> const& point, T includes_object_id) const
			-> bool
			requires std::is_invocable_r_v<bool, T, object_id_t>
		{
			for(std::size_t i = 0, last = obstacles_inv.size(); i < last; ++i) {
				if(includes_object_id(i)) {
					auto const& p = obstacles_inv[i];
					if(access(p.first).inside(point, p.second)) {
						return true;
					}
				}
			}
			return false;
		}
	};

	ObstacleSet(double grow_radius)
		: grow_radius{grow_radius}
	{}
	auto grown_view() const noexcept
		-> View<ObstacleGeometryAccessor_Grown>
	{
		return {obstacles_inv};
	}
	auto raw_view() const noexcept
		-> View<ObstacleGeometryAccessor_Raw>
	{
		return {obstacles_inv};
	}
	template<typename Accessor>
	auto size(object_id_t object_id) const
		-> Vertex<double, 3>
	{
		return Accessor::access(class_lookup[object_id]).bounding_box_size;
	}
	auto raw_size(object_id_t object_id) const
		-> Vertex<double, 3>
	{
		return size<ObstacleGeometryAccessor_Raw>(object_id);
	}
	auto grown_size(object_id_t object_id) const
		-> Vertex<double, 3>
	{
		return size<ObstacleGeometryAccessor_Grown>(object_id);
	}

	auto add_box_class(double length_x, double length_y, double length_z)
		-> class_id_t
	{
		using key_t = Vertex<double,3>;
		key_t key{length_x, length_y, length_z};
		auto it = know_boxes.find(key);
		if(it == know_boxes.end()) {
			geometries.push_back(
				std::make_unique<ObstacleClass>(
					  make_simple_cube<      true, false>(length_x, length_y, length_z)
					, make_simple_grown_cube<true, false>(length_x, length_y, length_z, grow_radius, grow_stacks, grow_sectors)
				)
			);
			it = know_boxes.insert(
				std::pair<key_t, class_id_t>{key, geometries.back().get()}
			).first;
		}
		return it->second;
	}
	auto add_cylinder_class(double radius, double height)
		-> class_id_t
	{
		using key_t = Vertex<double,2>;
		key_t key{radius, height};
		auto it = know_cylinders.find(key);
		if(it == know_cylinders.end()) {
			geometries.push_back(std::make_unique<ObstacleClass>(
				  make_simple_cylinder<      true, false>(radius, -height / 2.0, height / 2.0             , cylinder_sectors, true, true)
				, make_simple_grown_cylinder<true, false>(radius, -height / 2.0, height / 2.0, grow_radius, cylinder_sectors, grow_stacks)
			));
			it = know_cylinders.insert(
				std::pair<key_t, class_id_t>{key, geometries.back().get()}
			).first;
		}
		return it->second;
	}

	auto add_obstacle(class_id_t class_id, Transform const& T = {})
		-> object_id_t
	{
		std::size_t object_id = obstacles.size();
		obstacles.emplace_back(    class_id, T         );
		obstacles_inv.emplace_back(class_id, T.invert());
		class_lookup.push_back(    class_id);
		return object_id;
	}
	void clear_obstacles() {
		obstacles.clear();
		obstacles_inv.clear();
		class_lookup.clear();
	}
	auto transform(object_id_t object_id) const
		-> Transform const&
	{
		return obstacles[object_id].second;
	}
	void set_transform(object_id_t object_id, Transform const& T) {
		obstacles[    object_id].second = T;
		obstacles_inv[object_id].second = T.invert();
	}
};
