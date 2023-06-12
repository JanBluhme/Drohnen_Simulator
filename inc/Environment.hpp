#pragma once
#include "environment/TaxiGuests.hpp"
#include "environment/Obstacles.hpp"
#include "environment/Robots.hpp"
#include "client_server/make_command_set.hpp"
#include "environment_models/RobotMovementModelAcceleration.hpp"
#include "environment_models/RobotStateIntegration.hpp"
#include "robo_commands.hpp"
#include "config/TaxiGuest.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <random>
#include <thread>

#define VCHECK(v)                    \
	do {                             \
		assert(!std::isinf((v)[0])); \
		assert(!std::isinf((v)[1])); \
		assert(!std::isnan((v)[0])); \
		assert(!std::isnan((v)[1])); \
	} while(false)

namespace robo {

struct Environment {
	using CommandSet              = make_command_set_from_variant<robo::CommandSet>;
	using kinematic_model         = RobotMovementModelAcceleration;
	using robot_state_integration = RobotStateIntegration<kinematic_model>;
	using DebugLines              = std::vector<DebugLine>;

	struct GuiData {
		Robots                  robots;
		DebugLines              debug_lines;
		ObstacleSet::exchange_t obstacles;
		int                     speed_scale;
		bool                    is_paused;
		TaxiGuests              taxi_guests;
	};

	double                     delta_t_simulation;
	double                     delta_t_vision;
	int                        speed_scale;
	double                     time = 0.0;
	std::mutex                 mutex;
	bool                       is_running = true;
	bool                       is_paused  = false;
	RobotId                    current_id;
	Robots                     robots;
	std::vector<RobotView>     robot_views_buffer;
	std::vector<double>        distance_sensor_values_buffer;
	std::vector<Vision::Guest> visible_guests_buffer;
	std::mt19937               gen{};
	Obstacles                  obstacles;
	TaxiGuests                 taxi_guests;
	bool                       auto_kill_dead_robots = false;

	Environment(double delta_t_vision, double delta_t_simulation, int speed_scale)
		: delta_t_simulation{delta_t_simulation}
		, delta_t_vision{delta_t_vision}
		, speed_scale{speed_scale}
	{
// 		obstacles.add_random_N(gen, 64);
//
// 		for(std::size_t i = 0; i < 50; ++i) {
// 			taxi_guests.guests.push_back(taxi_guests.create_random_state(gen, obstacles));
// 		}
	}

	void move_obstacle(std::size_t id, Vertex<double, 2> new_position) {
		std::lock_guard<std::mutex> lock{mutex};
		obstacles.move(id, new_position);
		taxi_guests.validate(gen, obstacles);
	}
	auto closest_obstacle(Vertex<double, 3> const& position)
		-> std::optional<std::size_t>
	{
		std::lock_guard<std::mutex> lock{mutex};
		return obstacles.closest(position);
	}
	void clear_obstacles() {
		std::lock_guard<std::mutex> lock{mutex};
		obstacles.clear();
	}
	void clear_guests() {
		std::lock_guard<std::mutex> lock{mutex};
		for(auto& robot : robots) {
			robot.taxi_guest = {};
		}
		taxi_guests.clear();
	}
	void populate_guests(std::size_t N) {
		std::lock_guard<std::mutex> lock{mutex};
		for(std::size_t i = 0; i < N; ++i) {
			taxi_guests.guests.push_back(taxi_guests.create_random_state(gen, obstacles));
		}
	}
	void populate_obstacles(std::size_t N) {
		std::lock_guard<std::mutex> lock{mutex};
		obstacles.add_random_N(gen, N);
		taxi_guests.validate(gen, obstacles);
	}
	auto create_random_obstacle(ObstacleSet::class_id_t class_id)
		-> bool
	{
		std::lock_guard<std::mutex> lock{mutex};
		bool r = obstacles.add_random(gen, class_id);
		taxi_guests.validate(gen, obstacles);
		return r;
	}

	void move_robot(robo::RobotId const& id, Vertex<double, 2> new_position) {
		std::lock_guard<std::mutex> lock{mutex};
		Robot*                      robot = robots.find(id);
		if(robot) {
			robot->kinematics.position = new_position;
		}
	}

	template<typename debug_line_selector>
		requires std::is_invocable_r_v<bool, debug_line_selector, RobotId>
	auto get_gui_data(debug_line_selector selector)
		-> GuiData
	{
		std::lock_guard<std::mutex> lock{mutex};
		auto convert_debug_lines = [&]() {
			DebugLines result;
			result.reserve(robots.size());
			for(auto const& robot : robots) {
				if(selector(robot.id)) {
					auto const& src = robot.debug_lines;
					result.insert(result.end(), src.begin(), src.end());
				}
			}
			return result;
		};
		return {
			robots, convert_debug_lines(), obstacles.fix.obstacles, speed_scale, is_paused, taxi_guests
		};
	}

	auto closest_robot(Vertex<double, 3> const& position)
		-> std::optional<std::pair<double, robo::RobotId>>
	{
		std::lock_guard<std::mutex> lock(mutex);
		return robots.closest(position);
	}

	void reset() {
		std::lock_guard<std::mutex> lock{mutex};
		current_id = RobotId{};
		robots.clear();
	}

	void kill() {
		std::lock_guard<std::mutex> lock{mutex};
		is_running = false;
	}

	void toggle_pause() {
		std::lock_guard<std::mutex> lock{mutex};
		is_paused = !is_paused;
	}
	void set_pause(bool is_paused) {
		std::lock_guard<std::mutex> lock{mutex};
		this->is_paused = is_paused;
	}
	auto get_fps_vision()
		-> double
	{
		std::lock_guard<std::mutex> lock{mutex};
		return 1.0 / delta_t_vision;
	}
	auto get_fps_simulation()
		-> double
	{
		std::lock_guard<std::mutex> lock{mutex};
		return 1.0 / delta_t_simulation;
	}
	void set_fps_vision(double fps) {
		std::lock_guard<std::mutex> lock{mutex};
		delta_t_vision = 1.0 / fps;
	}
	void set_fps_simulation(double fps) {
		std::lock_guard<std::mutex> lock{mutex};
		delta_t_simulation = 1.0 / fps;
	}
	
	void erase_dead_robots() {
		robots.erase(
			std::remove_if(
				  robots.begin()
				, robots.end()
				, [&](Robot const& robot) {
					return robot.killed;
				}
			)
			, robots.end()
		);
	}
	void set_autokill_dead_robots(bool is_autokill_dead_robots) {
		std::lock_guard<std::mutex> lock{mutex};
		this->auto_kill_dead_robots = is_autokill_dead_robots;
		if(this->auto_kill_dead_robots) {
			erase_dead_robots();
		}
	}
	bool get_autokill_dead_robots() {
		std::lock_guard<std::mutex> lock{mutex};
		return auto_kill_dead_robots;
	}
	void set_speed_scale(int speed_scale) {
		std::lock_guard<std::mutex> lock{mutex};
		this->speed_scale = std::clamp(speed_scale, -4, 8);
	}
	void add_speed_scale(int delta) {
		std::lock_guard<std::mutex> lock{mutex};
		speed_scale = std::clamp(speed_scale + delta, -4, 8);
	}
	void set_robot_pause(robo::RobotId const& id, bool is_pause) {
		std::lock_guard<std::mutex> lock{mutex};
		Robot*                      robot = robots.find(id);
		if(robot) {
			robot->is_paused = is_pause;
		}
	}
	void toggle_robot_pause(robo::RobotId const& id) {
		std::lock_guard<std::mutex> lock{mutex};
		Robot*                      robot = robots.find(id);
		if(robot) {
			robot->is_paused = !robot->is_paused;
		}
	}

	auto scaled_delta_t(double t) const {
		return t * std::pow(2.0, speed_scale);
	}

	void update(bool override_pause) {
		std::lock_guard<std::mutex> lock{mutex};
		double dt = scaled_delta_t(delta_t_simulation);
		time += dt;
		obstacles.update(robots);
		taxi_guests.update(dt, robots);
		for(std::size_t i = 0, last = robots.size(); i != last; ++i) {
			auto& robot = robots[i];
			Robot::Kinematics old_kin = robot.kinematics;
			if(!is_paused && !robot.is_paused) {
				robot_state_integration::step(robot, dt);
				obstacles.fix_robot_position(gen, robot, i, old_kin);
			}
			obstacles.update_robot_rays(robot, i);
			robot.time_since_last_vision += dt;
		}
	}

	auto handle(RegisterRobotCommand::Request const& request)
		-> RegisterRobotCommand::Response
	{
		using Response = RegisterRobotCommand::Response;
		std::lock_guard<std::mutex> lock{mutex};
		if(is_running) {
			if(robots.empty()) {
				current_id       = RobotId{};
			}
			current_id                            = current_id.next();
			robots.emplace_back();
			Robot& robot    = robots.back();
			robot.id        = current_id;
			robot.name      = request.name;
			robot.reference = LocalVelocityReference{{0.0, 0.0}, 0.0};
			std::vector<double> sensor_angles;
			sensor_angles.reserve(Robot::sensor_angles.size());
			std::copy(
				  Robot::sensor_angles.begin()
				, Robot::sensor_angles.end()
				, std::back_inserter(sensor_angles)
			);
			return Response{
				Response::Result{
					RobotDescriptor{
						RobotDescriptor::KinematicModel{
							  config::Robot::velocity_max_x
							, config::Robot::velocity_max_y
							, config::Robot::angular_velocity_max
							, config::Robot::acceleration_max_x
							, config::Robot::acceleration_max_y
							, config::Robot::acceleration_max_angular
						}
						, RobotDescriptor::BodyModel{
							  config::Robot::mass
							, config::Robot::radius
							, sensor_angles
						}
						, RobotDescriptor::GuestModel{
							  config::TaxiGuest::max_pick_distance
							, config::TaxiGuest::max_drop_distance
							, config::TaxiGuest::max_pick_velocity
							, config::TaxiGuest::max_drop_velocity
						}
						, RobotDescriptor::VisibilityModel {
							  config::Robot::max_visibility_distance
							, config::TaxiGuest::max_visibility_distance
						}
					}
					, current_id
				}
			};
		}
		return Response{};
	}
	auto handle(DeregisterRobotCommand::Request const& request)
		-> DeregisterRobotCommand::Response
	{
		using Response = DeregisterRobotCommand::Response;
		using Result   = Response::Result;
		std::lock_guard<std::mutex> lock{mutex};
		Robot*                      robot = robots.find(request.id);
		if(!robot) {
			return {Result::UNKNOWN_ROBOT};
		}
		std::cerr << "Deregister robot: " << *robot << '\n';
		Response r{Result::SUCCESS};
		robot->killed = true;
		if(auto_kill_dead_robots) {
			erase_dead_robots();
		}
		return r;
	}
	auto handle(LocalVelocityCommand::Request const& request)
		-> LocalVelocityCommand::Response
	{
		using Response = LocalVelocityCommand::Response;
		using Result   = Response::Result;
		std::lock_guard<std::mutex> lock{mutex};
		Robot*                      robot = robots.find(request.id);
		if(!robot) {
			return Response{Result::UNKNOWN_ROBOT};
		}
		if(    std::isnan(request.velocity[0])
			|| std::isnan(request.velocity[1])
			|| std::isnan(request.angular_velocity)
			|| std::isinf(request.velocity[0])
			|| std::isinf(request.velocity[1])
			|| std::isinf(request.angular_velocity)
			|| std::abs(request.velocity[0]     ) > 1.05 * config::Robot::velocity_max_x
			|| std::abs(request.velocity[1]     ) > 1.05 * config::Robot::velocity_max_y
			|| std::abs(request.angular_velocity) > 1.05 * config::Robot::angular_velocity_max
		) {
			return Response{Result::KINEMATIC_LIMITS_EXCEEDED};
		}
		robot->reference = LocalVelocityReference{request.velocity, request.angular_velocity};
		return Response{Result::SUCCESS};
	}
	auto handle(LocalVelocityFixedFrameCommand::Request const& request)
		-> LocalVelocityFixedFrameCommand::Response
	{
		using Response = LocalVelocityFixedFrameCommand::Response;
		using Result   = Response::Result;
		std::lock_guard<std::mutex> lock{mutex};
		Robot*                      robot = robots.find(request.id);
		if(!robot) {
			return Response{Result::UNKNOWN_ROBOT};
		}
		double v_max = 1.05 * std::min(config::Robot::velocity_max_x, config::Robot::velocity_max_y);
		if(    std::isnan(request.velocity[0])
			|| std::isnan(request.velocity[1])
			|| std::isnan(request.angular_velocity)
			|| std::isinf(request.velocity[0])
			|| std::isinf(request.velocity[1])
			|| std::isinf(request.angular_velocity)
			|| std::abs(request.velocity[0]     ) > v_max
			|| std::abs(request.velocity[1]     ) > v_max
			|| std::abs(request.angular_velocity) > 1.05 * config::Robot::angular_velocity_max
		) {
			return Response{Result::KINEMATIC_LIMITS_EXCEEDED};
		}
		robot->reference = LocalVelocityFixedFrameReference{
			request.velocity, request.angular_velocity, robot->kinematics.orientation
		};
		return Response{Result::SUCCESS};
	}

	auto collides(Robot const& robot, Vertex<double, 2> const& start, Vertex<double, 2> const& end)
		-> QuerySegmentTraversableCommand::Response::Result
	{
		using Result = QuerySegmentTraversableCommand::Response::Result;
		Vertex<double, 3> start3 {
			start[0], start[1], config::Body::h1 / 2.0
		};
		Vertex<double, 3> end3 {
			end[0], end[1], config::Body::h1 / 2.0
		};
		if(obstacles.has_collision_robot(start3, end3, *robots.index(robot.id))) {
			return Result::BLOCKED_BY_ROBOT;
		}
		if(obstacles.has_collision_fix(start3, end3)) {
			return Result::BLOCKED_BY_OBSTACLE;
		}
		return Result::TRAVERSABLE;
	}

	auto handle(QuerySegmentTraversableCommand::Request const& request)
		-> QuerySegmentTraversableCommand::Response
	{
		using Response = QuerySegmentTraversableCommand::Response;
		using Result   = Response::Result;
		std::lock_guard<std::mutex> lock{mutex};
		auto                        robot = robots.find(request.id);
		if(!robot) {
			return Response{Result::UNKNOWN_ROBOT};
		}
		return Response{collides(*robot, request.start, request.end)};
	}

	auto handle(QueryVisionCommand::Request const& request)
		-> QueryVisionCommand::Response
	{
		using Response = QueryVisionCommand::Response;
		using Result   = Response::Result;
		std::unique_lock<std::mutex> lock{mutex};
		Robot*                       robot = robots.find(request.id);
		if(!robot) {
			return Response{Result::UNKNOWN_ROBOT};
		}
		double  vision_dt = scaled_delta_t(delta_t_vision);
		double& time_since_last_vision = robot->time_since_last_vision;
		if(time_since_last_vision < vision_dt) {
			double time_to_wait = vision_dt - time_since_last_vision;
			lock.unlock();
			std::this_thread::sleep_for(std::chrono::duration<double>{time_to_wait});
			lock.lock();
		}
		time_since_last_vision = 0.0;

		auto guest = [&](std::size_t idx)
			-> Vision::Guest
		{
			return {
				  taxi_guests.guests[idx].position
				, taxi_guests.guests[idx].target_position
			};
		};

		auto generate_taxi_guest_views = [&]()
			-> std::vector<Vision::Guest> const&
		{
			std::vector<std::size_t> visible_guests_idx = taxi_guests.guests_in_range(
				robot->kinematics.position, config::TaxiGuest::max_visibility_distance
			);
			visible_guests_buffer.clear();
			visible_guests_buffer.reserve(visible_guests_idx.size());
			for(std::size_t idx : visible_guests_idx) {
				visible_guests_buffer.push_back(guest(idx));
			}
			return visible_guests_buffer;
		};

		auto generate_robot_views = [&]()
			-> std::vector<RobotView> const&
		{
			robot_views_buffer.clear();
			robot_views_buffer.reserve(robots.size());
			for(auto const& r : robots) {
				Vertex<double,2> d = robot->kinematics.position - r.kinematics.position;
				double const d2_max = config::Robot::max_visibility_distance * config::Robot::max_visibility_distance;
				if(d*d <= d2_max) {
					robot_views_buffer.push_back(r.view());
				}
			}
			return robot_views_buffer;
		};
		auto generate_distance_sensor_values = [&]()
			-> std::vector<double> const&
		{
			distance_sensor_values_buffer.clear();
			distance_sensor_values_buffer.reserve(Robot::num_rays);
			std::copy(
				  robot->ray_distances.begin()
				, robot->ray_distances.end()
				, std::back_inserter(distance_sensor_values_buffer)
			);
			return distance_sensor_values_buffer;
		};

		return Response{
			  Result::SUCCESS
			, Vision{
				  generate_robot_views()
				, generate_taxi_guest_views()
				, generate_distance_sensor_values()
				, robot->taxi_guest ? guest(*robot->taxi_guest) : std::optional<Vision::Guest>{}
			  }
		};
	}

	auto handle(PickTaxiGuestCommand::Request const& request)
		-> PickTaxiGuestCommand::Response
	{
		using Response = PickTaxiGuestCommand::Response;
		using Result   = Response::Result;
		std::lock_guard<std::mutex> lock{mutex};
		Robot*                      robot = robots.find(request.id);
		if(!robot) {
			return Response{Result::UNKNOWN_ROBOT};
		}
		auto const& v = robot->kinematics.velocity;
		if(v*v > config::TaxiGuest::max_pick_velocity*config::TaxiGuest::max_pick_velocity) {
			return Response{
				Result::TOO_FAST_TO_PICK
			};
		}
		bool has_picked = taxi_guests.pick(*robot, config::TaxiGuest::max_pick_distance);
		std::optional<Vision::Guest> picked_guest;
		Result r = Result::NO_GUEST_IN_RANGE;
		if(has_picked) {
			auto& g = taxi_guests.guests[*robot->taxi_guest];
			picked_guest = Vision::Guest{
				g.position, g.target_position
			};
			r = Result::SUCCESS;
		}
		return Response{r, picked_guest};
	}

	auto handle(DropTaxiGuestCommand::Request const& request)
		-> DropTaxiGuestCommand::Response
	{
		using Response = DropTaxiGuestCommand::Response;
		using Result   = Response::Result;
		std::lock_guard<std::mutex> lock{mutex};
		Robot*                      robot = robots.find(request.id);
		if(!robot) {
			return Response{Result::UNKNOWN_ROBOT};
		}
		if(!robot->taxi_guest) {
			return Response{Result::NO_GUEST_TO_DROP, 0};
		}
		auto const& v = robot->kinematics.velocity;
		if(v*v > config::TaxiGuest::max_drop_velocity*config::TaxiGuest::max_drop_velocity) {
			return Response{
				Result::TOO_FAST_TO_DROP
			};
		}
		int drop_score = taxi_guests.drop(gen, obstacles, *robot, config::TaxiGuest::max_drop_distance);
		return Response{Result::SUCCESS, drop_score};
	}

	auto handle(SetDebugLinesCommand::Request const& request)
		-> SetDebugLinesCommand::Response
	{
		using Response = SetDebugLinesCommand::Response;
		using Result   = Response::Result;
		std::lock_guard<std::mutex> lock{mutex};
		Robot*                      robot = robots.find(request.id);
		if(!robot) {
			return Response{Result::UNKNOWN_ROBOT};
		}
		robot->debug_lines = request.lines;
		return Response{Result::SUCCESS};
	}
};

}   // namespace robo
