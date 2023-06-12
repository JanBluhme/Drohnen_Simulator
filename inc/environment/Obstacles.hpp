#pragma once
#include "environment_models/ObstacleSet.hpp"
#include "environment/Robots.hpp"
#include "config/Robot.hpp"
#include "config/Pitch.hpp"
#include <random>

namespace robo {

struct Obstacles {
	using class_ids_t = std::vector<ObstacleSet::class_id_t>;
	ObstacleSet             fix{   config::Robot::radius};
	ObstacleSet             robots{config::Robot::radius};
	ObstacleSet::class_id_t robot_class = robots.add_cylinder_class(config::Robot::radius, config::Body::h1);
	class_ids_t             class_ids{
		  fix.add_box_class(0.2, 0.2, 0.5)
		, fix.add_box_class(0.3, 0.5, 1.0)
		, fix.add_cylinder_class(0.2, 0.2)
		, fix.add_cylinder_class(0.3, 0.8)
	};
	bool                    movable_obstacles = false;

	void clear() {
		fix.clear_obstacles();
	}
	void add(ObstacleSet::class_id_t class_id, Transform const& T) {
		ObstacleSet::object_id_t id = fix.add_obstacle(class_id, T);
	}

	template<typename Gen>
	auto add_random(Gen& gen, ObstacleSet::class_id_t class_id)
		-> bool
	{
		static std::array const obstacle_rot_z{
			  Transform::rotate_z(0.0 * M_PI_4)
			, Transform::rotate_z(1.0 * M_PI_4)
			, Transform::rotate_z(2.0 * M_PI_4)
			, Transform::rotate_z(3.0 * M_PI_4)
			, Transform::rotate_z(4.0 * M_PI_4)
			, Transform::rotate_z(5.0 * M_PI_4)
			, Transform::rotate_z(6.0 * M_PI_4)
			, Transform::rotate_z(7.0 * M_PI_4)
		};

		std::uniform_real_distribution<double>     dist_x{-config::Pitch::width  / 2.0, config::Pitch::width  / 2.0};
		std::uniform_real_distribution<double>     dist_y{-config::Pitch::height / 2.0, config::Pitch::height / 2.0};
		std::uniform_int_distribution<std::size_t> dist_z{0, obstacle_rot_z.size() - 1};
		for(std::size_t tries = 0; tries < 100; ++tries) {
			Transform T =
				  Transform::translate({dist_x(gen), dist_y(gen), class_id->raw.bounding_box_size[2] / 2.0})
				* obstacle_rot_z[dist_z(gen)]
			;
			if(is_placeable(class_id, T)) {
				add(class_id, T);
				return true;
			}
		}
		return false;
	}

	template<typename Gen>
	void add_random_N(Gen& gen, std::size_t N) {
		std::size_t tries = 0;
		std::discrete_distribution<std::size_t> dist{10.0, 2.0, 8.0, 1.0};
		for(std::size_t i = 0; i < N && tries < 100; ++i) {
			ObstacleSet::class_id_t class_id = class_ids[dist(gen)];
			++tries;
			if(add_random(gen, class_id)) {
				tries = 0;
			}
		}
	}

	void move(std::size_t id, Vertex<double, 2> new_position) {
		Transform T = fix.transform(id);
		T.T[0] = new_position[0];
		T.T[1] = new_position[1];
		fix.set_transform(id, T);
	}

	auto is_placeable(ObstacleSet::class_id_t class_id, Transform const& transform) const
		-> bool
	{
		auto view = fix.grown_view();
		for(auto const& triangle : class_id->grown.mesh_data.triangles()) {
			Vertex<double,3> A = transform.position(triangle.A);
			Vertex<double,3> B = transform.position(triangle.B);
			Vertex<double,3> C = transform.position(triangle.C);
			if(    view.intersects(Segment3{A, B - A})
				|| view.intersects(Segment3{B, C - B})
				|| view.intersects(Segment3{C, A - C})
			) {
				return false;
			}
		}
		return true;
	}
	auto is_robot_placeable(Vertex<double, 2> const& position) const
		-> bool
	{
		return is_placeable(robot_class, Transform::translate(position[0], position[1], 0.0));
	}
	auto closest(Vertex<double, 3> const& position) const
		-> std::optional<std::size_t>
	{
		std::pair<double, std::size_t> result{
			std::numeric_limits<double>::max(), 0
		};
		for(std::size_t i = 0, end = fix.obstacles_inv.size(); i != end; ++i) {
			auto const& p = fix.obstacles_inv[i];
			if(p.first->grown.inside(position, p.second)) {
				return i;
			}
		}
		return {};
	}
	void update(Robots& robots) {
		this->robots.obstacles.resize(    robots.size());
		this->robots.obstacles_inv.resize(robots.size());
		this->robots.class_lookup.resize( robots.size());
		for(std::size_t i = 0, last = robots.size(); i < last; ++i) {
			this->robots.class_lookup[i] = robot_class;
			Vertex<double,2> const& pos = robots[i].kinematics.position;
			Vertex<double,3> T{pos[0], pos[1], robot_class->raw.bounding_box_size[2] / 2.0};
			this->robots.set_transform(i, Transform::translate(T) * Transform::rotate_z(robots[i].kinematics.orientation));
			this->robots.obstacles[    i].first = robot_class;
			this->robots.obstacles_inv[i].first = robot_class;
		}
	}


	// returns whether robot position was changed...
	template<typename Gen>
	auto fix_robot_position(Gen& gen, Robot& robot, std::size_t index, Robot::Kinematics const& old_kinematics) const
		-> bool
	{
		auto acceptor = [&](std::size_t idx) {
			return index != idx;
		};
		auto is_valid_position = [&](Vertex<double, 2> const& pos) {
			Vertex<double, 3> pos3 {
				pos[0], pos[1], config::Body::h1 / 2.0
			};
			return !robots.grown_view().inside(pos3, acceptor)
				&& !fix.grown_view().inside(   pos3);
		};
		auto try_fix = [&](double displacement)
			-> std::optional<Vertex<double,2>>
		{
			std::uniform_real_distribution<double> dist{-displacement, displacement};
			Vertex<double,2> A = old_kinematics.position + Vertex<double,2>{dist(gen), dist(gen)};
			for(std::size_t i = 0; i < 10; ++i) {
				if(is_valid_position(A)) {
					return A;
				}
			}
			return {};
		};
		if(is_valid_position(robot.kinematics.position)) {
			return false;
		}
		double displacement = 0.02;
		std::optional<Vertex<double,2>> opt_pos;
		do {
			opt_pos = try_fix(displacement);
			displacement += 0.02;
		} while(!opt_pos);
		robot.kinematics.position = *opt_pos;
		return true;
	}

	// returns whether robot position was changed...
	template<typename Gen>
	auto fix_guest_position(Gen& gen, Vertex<double,2>& guest_position) const
		-> bool
	{
		Vertex<double,2> old_position = guest_position;

		auto is_valid_position = [&](Vertex<double, 2> const& pos) {
			Vertex<double, 3> pos3 {
				pos[0], pos[1], config::Body::h1 / 2.0
			};
			return !fix.grown_view().inside(pos3);
		};
		auto try_fix = [&](double displacement)
			-> std::optional<Vertex<double,2>>
		{
			std::uniform_real_distribution<double> dist{-displacement, displacement};
			Vertex<double,2> A = old_position + Vertex<double,2>{dist(gen), dist(gen)};
			for(std::size_t i = 0; i < 10; ++i) {
				if(is_valid_position(A)) {
					return A;
				}
			}
			return {};
		};
		if(is_valid_position(guest_position)) {
			return false;
		}
		double displacement = 0.02;
		std::optional<Vertex<double,2>> opt_pos;
		do {
			opt_pos = try_fix(displacement);
			displacement += 0.02;
		} while(!opt_pos);
		guest_position = *opt_pos;
		return true;
	}

	void update_robot_rays(Robot& robot, std::size_t index) const {
		auto obstacles_view = fix.raw_view();
		auto robots_view    = robots.raw_view();
		for(std::size_t i = 0; i < Robot::num_rays; ++i) {
			Transform const& T = robots.transform(index);
			Ray3 ray = Ray3{
				  T.position( robot.rays[i].point)
				, T.direction(robot.rays[i].direction)
			};
			auto o_lambda_fix    = obstacles_view.intersect(ray);
			auto acceptor = [&](std::size_t idx) {
				return index != idx;
			};
			auto o_lambda_robots = robots_view.intersect(ray, acceptor);
			robot.ray_distances[i] = std::min(
				  o_lambda_fix    ? *o_lambda_fix    : std::numeric_limits<double>::max()
				, o_lambda_robots ? *o_lambda_robots : std::numeric_limits<double>::max()
			);
		}
	}
	auto has_collision_fix(Vertex<double, 3> const& start, Vertex<double, 3> const& end) const
		-> bool
	{
		return fix.grown_view().intersects(Segment3{start, end - start});
	}
	auto has_collision_robot(Vertex<double, 3> const& start, Vertex<double, 3> const& end, std::size_t robot_index) const
		-> bool
	{
		auto acceptor = [&](std::size_t index) {
			return robot_index != index;
		};
		return robots.grown_view().intersects(Segment3{start, end - start}, acceptor);
	}
};

} /* namespace robo */
