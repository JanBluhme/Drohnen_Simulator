#pragma once
#include "environment/Obstacles.hpp"
#include "environment/Robots.hpp"
#include "config/TaxiGuest.hpp"

namespace robo {

struct TaxiGuests {
	struct GuestState {
		Vertex<double, 2>      position;
		double                 rotation = 0.0;
		Vertex<double, 2>      target_position;
		int                    score_on_arrival;
		bool                   done = false;
		double                 phi_z = 0.0;
		double                 height = 0.0;
		std::optional<RobotId> bound_to_robot{};
	};
	std::vector<GuestState> guests;
	double                  time;

	template<typename Gen>
	void validate(Gen& gen, Obstacles const& obstacles) {
		for(auto& g : guests) {
			obstacles.fix_guest_position(gen, g.position);
			obstacles.fix_guest_position(gen, g.target_position);
		}
	}

	template<typename Gen>
	auto create_random_state(Gen& gen, Obstacles const& obstacles)
		-> GuestState
	{
		auto random_position = [&]() {
			std::uniform_real_distribution<double> dist_x{-config::Pitch::width  / 2.0, config::Pitch::width  / 2.0};
			std::uniform_real_distribution<double> dist_y{-config::Pitch::height / 2.0, config::Pitch::height / 2.0};
			while(true) {
				Vertex<double, 2> p{dist_x(gen), dist_y(gen)};
				if(obstacles.is_robot_placeable(p)) {
					return p;
				}
			}
		};

		Vertex<double, 2> position = random_position();
		Vertex<double, 2> target   = random_position();
		Vertex<double, 2> d = position - target;
		double points_for_distance = d.length() * config::TaxiGuest::points_per_meter;
		double tip_ratio = std::uniform_real_distribution<double>{
			  config::TaxiGuest::min_tip
			, config::TaxiGuest::max_tip
		}(gen);
		double points = points_for_distance * (1.0 + tip_ratio);
		int    score_on_arrival = std::round(points);
		std::uniform_real_distribution<double> dist_phi{0.0, 2.0 * M_PI};
		double phi_z = dist_phi(gen);
		return {
			  position
			, dist_phi(gen)
			, target
			, score_on_arrival
			, false
			, phi_z
		};
	}
	auto guests_in_range(Vertex<double,2> const& robot_position, double max_distance)
		-> std::vector<std::size_t>
	{
		std::vector<std::size_t> result;
		for(std::size_t i = 0; i < guests.size(); ++i) {
			auto const& g = guests[i];
			if(!g.bound_to_robot && !g.done) {
				Vertex<double,2> d = robot_position - g.position;
				if(d*d < max_distance*max_distance) {
					result.push_back(i);
				}
			}
		}
		return result;
	}
	auto pick(Robot& robot, double max_pick_distance)
		-> bool
	{
		if(robot.taxi_guest) {
			return false;
		}
		double distance2 = std::numeric_limits<double>::max();
		std::size_t idx = 0;
		for(std::size_t i = 0; i < guests.size(); ++i) {
			auto const& g = guests[i];
			if(!g.bound_to_robot && !g.done) {
				Vertex<double,2> d = robot.kinematics.position - g.position;
				if(d*d < distance2) {
					idx       = i;
					distance2 = d * d;
				}
			}
		}
		if(distance2 <= max_pick_distance * max_pick_distance) {
			guests[idx].bound_to_robot = robot.id;
			robot.taxi_guest = idx;
			return true;
		}
		return false;
	}
	template<typename Gen>
	auto drop(Gen& gen, Obstacles const& obstacles, Robot& robot, double max_drop_distance)
		-> int
	{
		if(!robot.taxi_guest) {
			return 0.0;
		}
		GuestState& g = guests[*robot.taxi_guest];
		Vertex<double,2> d = robot.kinematics.position - g.target_position;
		if(d*d <= max_drop_distance) {
			robot.taxi_guest = {};
			g.bound_to_robot = {};
			int score = g.score_on_arrival;
			g.done = true;
// 			g = create_random_state(gen, obstacles);
			robot.score += score;
			return score;
		}
		return 0.0;
	}
	void update(double dt, Robots const& robots) {
		time += dt;
		std::size_t i = 0;
		for(auto& g : guests) {
			if(g.bound_to_robot && !robots.index(*g.bound_to_robot)) {
				g.bound_to_robot = {};
			}
			if(g.bound_to_robot) {
				std::size_t idx = *robots.index(*g.bound_to_robot);
				g.position[0] = robots[idx].kinematics.position[0];
				g.position[1] = robots[idx].kinematics.position[1];
				g.rotation -= 2.0 * dt;
				g.height = config::Body::h1;
				++i;
			} else {
				g.rotation += 1.0 * dt;
				g.height = (std::sin(time * 2.0 + g.phi_z) + 1.0) * 0.5 * 0.1;
			}
		}
	}
	void clear() {
		guests.clear();
	}
};

} /* namespace robo*/
