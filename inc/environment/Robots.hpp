#pragma once
#include "Robot.hpp"

namespace robo {

struct Robots
	: std::vector<Robot>
{
	auto index(RobotId robot_id) const
		-> std::optional<std::size_t>
	{
		for(std::size_t i = 0, last = size(); i < last; ++i) {
			std::vector<Robot> const& robots = *this;
			if(robots[i].id == robot_id) {
				return i;
			}
		}
		return {};
	}
	auto find(RobotId id) const
		-> Robot const*
	{
		for(Robot const *first = data(), *last = data() + size()
			; first != last; ++first
		) {
			if(first->id == id) {
				return first;
			}
		}
		return nullptr;
	}
	auto find(RobotId id)
		-> Robot*
	{
		for(Robot *first = data(), *last = data() + size()
			; first != last; ++first
		) {
			if(first->id == id) {
				return first;
			}
		}
		return nullptr;
	}
	auto closest(Vertex<double, 3> const& position)
		-> std::optional<std::pair<double, robo::RobotId>>
	{
		Vertex<double, 2>       p2{position[0], position[1]};
		if(empty()) {
			return {};
		}
		std::pair<double, robo::RobotId> result{
			std::numeric_limits<double>::max(), RobotId{0}
		};
		for(auto const& robot : *this) {
			double d = (robot.kinematics.position - p2).length();
			if(d < result.first) {
				result.first  = d;
				result.second = robot.id;
			}
		}
		return result;
	}
};

} /* namespace robo */
