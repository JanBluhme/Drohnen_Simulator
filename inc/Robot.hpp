#pragma once
#include "math/Vertex.hpp"
#include "math/r3/Triangle.hpp"
#include "robo_commands.hpp"
#include "config/Body.hpp"

namespace robo {
struct Robot {
	using velocity_command_t = std::variant<
		  LocalVelocityReference
		, LocalVelocityFixedFrameReference
	>;

	constexpr static std::size_t                   num_rays = 16;
	inline static std::array<double, num_rays>     sensor_angles {
		[]() {
			std::array<double, num_rays> sensor_angles;
			for(std::size_t i = 0; i < num_rays; ++i) {
				sensor_angles[i] = 2.0 * M_PI * i / num_rays;
			}
			return sensor_angles;
		}()
	};
	inline static std::array<Ray3, num_rays> const rays = []() {
		std::array<Ray3, num_rays> rays;
		for(std::size_t i = 0; i < num_rays; ++i) {
			Vertex<double,2> xy = polar(1.0, sensor_angles[i]);
			rays[i].point     = Vertex<double,3>{0.0  , 0.0  , 0.0};
			rays[i].direction = Vertex<double,3>{xy[0], xy[1], 0.0 };
		}
		return rays;
	}();

	struct Kinematics {
		Vertex<double,2> position;
		Vertex<double,2> velocity;
		double           orientation;
		double           angular_velocity;
		Vertex<double,4> wheel_turn_angle;  // integral of wheel omegas

		template<typename OS>
		friend
		auto operator<<(OS& os, Kinematics const& kinematics)
			-> OS&
		{
			os  << "[position: "          << kinematics.position
				<< ", velocity: "         << kinematics.velocity
				<< ", orientation: "      << kinematics.orientation
				<< ", angular_velocity: " << kinematics.angular_velocity
				<< ']'
			;
			return os;
		}
	};
	RobotId                      id;
	std::string                  name;
	Kinematics                   kinematics;
	velocity_command_t           reference;
	std::vector<DebugLine>       debug_lines;
	bool                         is_paused = false;
	double                       time_since_last_vision = 0.0;
	std::array<double, num_rays> ray_distances;
	std::optional<std::size_t>   taxi_guest;
	int                          score;
	bool                         killed = false;

	template<typename OS>
	friend
	auto operator<<(OS& os, Robot const& robot)
		-> OS&
	{
		os  << "[id:: "         << robot.id
			<< ", kinematics: " << robot.kinematics
			<< ", reference: "
		;
		std::visit(
			[&](auto const& r) {
				os << r;
			}
			, robot.reference
		);
		return os;
	}

	auto view() const
		-> RobotView
	{
		return {
			  id
			, name
			, kinematics.position
			, kinematics.velocity
			, kinematics.orientation
			, kinematics.angular_velocity
			, score
		};
	}
};

} /* name robo */
