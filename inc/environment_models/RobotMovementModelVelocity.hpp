#pragma once
#include "RobotVelocity.hpp"
#include "Robot.hpp"
#include "math/angle_util.hpp"
#include "config/Simulation.hpp"
#include "config/Robot.hpp"
#include <cassert>

namespace robo {

struct RobotMovementModelVelocity {
#if 0
	static RobotVelocity position_control(Robot const& robot, double dt) {
		assert(std::holds_alternative<GlobalPositionReference>(robot.reference));
		GlobalPositionReference const& reference = std::get<GlobalPositionReference>(robot.reference);
		double phi_error = sm::normalize_angle_relative(
			  sm::normalize_angle_relative(reference.orientation)
			- sm::normalize_angle_relative(robot.kinematics.orientation)
		);
		double k_phi = 10.0;
		double omega = k_phi * phi_error;
		if(std::abs(omega) > config::Robot::angular_velocity_max) {
			omega *= config::Robot::angular_velocity_max / std::abs(omega);
		}
		Vertex<double,2> pos_error = reference.position - robot.kinematics.position;
		double k_pos = 10.0;
		Vertex<double,2> velocity = k_pos * pos_error;
		double v = velocity.length();
		double v_max = config::Robot::velocity_max;
		if(v > v_max) {
			velocity *= v_max/v;
			v = v_max;
		}
		RobotVelocity result;
		result.local = rotate(
			velocity
			, -robot.kinematics.orientation
			- robot.kinematics.angular_velocity * dt
		);
		result.global = rotate(result.local, robot.kinematics.orientation);
		result.angular_velocity = omega;
		return result;
	}
#endif
	static void update_velocities(RobotVelocity const& reference_velocities, Robot::Kinematics& kinematics) {
		kinematics.velocity = reference_velocities.global;
		kinematics.angular_velocity = reference_velocities.angular_velocity;
	}
};

} /** namespace robo */
