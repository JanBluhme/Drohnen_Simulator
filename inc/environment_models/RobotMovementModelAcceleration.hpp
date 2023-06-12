#pragma once
#include "config/Robot.hpp"
#include "config/Simulation.hpp"
#include "RobotVelocity.hpp"
#include "Robot.hpp"
#include "math/angle_util.hpp"
#include <cassert>

namespace robo {

struct RobotMovementModelAcceleration {
#if 0
	static RobotVelocity position_control(Robot const& robot, double dt) {
		assert(std::holds_alternative<GlobalPositionReference>(robot.reference));
		GlobalPositionReference const& reference = std::get<GlobalPositionReference>(robot.reference);
		auto calc_vel = [](double thresh, double a, double s) {
			auto calc_vel2 = [&](double s) {
				double x = 2.0 * a * s;
				return x < 0
					? -std::sqrt(x)
					:  std::sqrt(x)
				;
			};
			if(s < thresh) {
				return s * calc_vel2(thresh) / thresh;
			}
			return calc_vel2(s);
		};
		double phi_error = sm::normalize_angle_relative(
			  sm::normalize_angle_relative(reference.orientation)
			- sm::normalize_angle_relative(robot.kinematics.orientation)
		);
		double omega = 0.5*calc_vel(25.0*sm::to_radians, config::Robot::acceleration_angular_max, phi_error);
		if(std::abs(omega) > config::Robot::angular_velocity_max) {
			omega *= config::Robot::angular_velocity_max / std::abs(omega);
		}
		Vertex<double,2> pos_error = reference.position - robot.kinematics.position;
		double               d         = pos_error.length();
		double               v = 0.9*calc_vel(0.25, config::Robot::acceleration_max, d);
		Vertex<double,2> velocity  = std::abs(d) > 1e-3
			? pos_error * (v/d)
			: pos_error
		;
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
	static void update_velocities(RobotVelocity const& reference_velocities, Robot::Kinematics& kinematics, double dt) {
		double angular_velocity_error   = reference_velocities.angular_velocity - kinematics.angular_velocity;
		double angular_acceleration     = angular_velocity_error / dt;
		Vertex<double,2> velocity_error = reference_velocities.global - kinematics.velocity;
		Vertex<double,2> acceleration   = velocity_error / dt;
		Vertex<double,2> acceleration_loc = rotate(acceleration, -kinematics.orientation);

		double scale = 1.0;

		auto do_scale = [&](double value, double max) {
			if(std::abs(value) > max) {
				scale = std::min(scale, max/std::abs(value));
			}
		};
		do_scale(angular_acceleration, config::Robot::acceleration_max_angular);
		do_scale(acceleration_loc[0] , config::Robot::acceleration_max_x      );
		do_scale(acceleration_loc[1] , config::Robot::acceleration_max_y      );
		angular_acceleration *= scale;
		acceleration         *= scale;

		kinematics.angular_velocity +=             dt  * angular_acceleration;
		kinematics.velocity         +=             dt  * acceleration;
		kinematics.position         += (0.5 * dt * dt) * acceleration;
		kinematics.orientation      += (0.5 * dt * dt) * angular_acceleration;
	}
};

} /** namespace robo */
