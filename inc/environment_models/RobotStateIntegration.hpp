#pragma once

#include "math/angle_util.hpp"
#include "environment_models/RobotVelocity.hpp"
#include "robo_commands.hpp"
#include "config/Wheel.hpp"
#include "config/Simulation.hpp"
#include "config/Pitch.hpp"
#include "Robot.hpp"
#include <iostream>
#include <random>

namespace robo {

template<typename kinematic_model>
struct RobotStateIntegration {
	static RobotVelocity get_reference_velocities(Robot& robot, double dt) {
		RobotVelocity result;
#if 0
		if(std::holds_alternative<GlobalVelocityReference>(robot.reference)) {
			GlobalVelocityReference const& reference = std::get<GlobalVelocityReference>(robot.reference);
			result.global           = reference.velocity;
			result.local            = rotate(result.global, -robot.kinematics.orientation);
			result.angular_velocity = reference.angular_velocity;
		} else if(std::holds_alternative<LocalVelocityReference>(robot.reference)) {
			LocalVelocityReference const& reference = std::get<LocalVelocityReference>(robot.reference);
			result.local            = reference.velocity;
			result.global           = rotate(result.local, robot.kinematics.orientation);
			result.angular_velocity = reference.angular_velocity;
		} else if(std::holds_alternative<GlobalPositionReference>(robot.reference)) {
			result = kinematic_model::position_control(robot, dt);
		}
#endif
		if(std::holds_alternative<LocalVelocityReference>(robot.reference)) {
			using vel_ref_t = LocalVelocityReference;
			vel_ref_t const& reference = std::get<vel_ref_t>(robot.reference);
			result.local               = reference.velocity;
			result.global              = rotate(result.local, robot.kinematics.orientation);
			result.angular_velocity    = reference.angular_velocity;
		}
		if(std::holds_alternative<LocalVelocityFixedFrameReference>(robot.reference)) {
			using vel_ref_t = LocalVelocityFixedFrameReference;
			vel_ref_t const& reference = std::get<vel_ref_t>(robot.reference);
			double delta_orientation = robot.kinematics.orientation - reference.fixed_orientation;
			result.local               = rotate(reference.velocity, -delta_orientation);
			result.global              = rotate(result.local, robot.kinematics.orientation);
			result.angular_velocity    = reference.angular_velocity;
		}
		return result;
	}

	static void state_check(Robot& robot, char const* error_msg) {
		if(    !robot.kinematics.position.is_finite()
			|| !robot.kinematics.velocity.is_finite()
			|| !std::isfinite(robot.kinematics.orientation)
			|| !std::isfinite(robot.kinematics.angular_velocity)
		) {
			std::cerr << "EE::RobotStateIntegration:Infinity detected (" << error_msg << "): " << robot << '\n';
			std::random_device rd{};
			robot.kinematics.position = {
				  std::uniform_real_distribution<double>{-config::Pitch::width/2, config::Pitch::width/2}(rd)
				, std::uniform_real_distribution<double>{-config::Pitch::height/2, config::Pitch::height/2}(rd)
			};
			robot.kinematics.velocity = {0,0};
			robot.kinematics.orientation = 0;
			robot.kinematics.angular_velocity = 0;
		}
	}
	
	static void step(Robot& robot, double dt) {
		state_check(robot, "State_0");
		auto& kinematics = robot.kinematics;
		RobotVelocity reference_velocities = get_reference_velocities(robot, dt);
		if(    !reference_velocities.local.is_finite()
			|| !reference_velocities.global.is_finite()
			|| !std::isfinite(reference_velocities.angular_velocity)
		) {
			std::cerr << "EE::RobotStateIntegration:Infinity detected (Reference): " << robot << '\n';
			reference_velocities.local = {0,0};
			reference_velocities.global = {0,0};
			reference_velocities.angular_velocity = 0;
		}
		
		kinematic_model::update_velocities(reference_velocities, robot.kinematics, dt);
		state_check(robot, "State_1");
		
		double omega = kinematics.angular_velocity;
		if(std::abs(omega) > 1e-3) {
			Vertex<double,2> n = ortho(kinematics.velocity)/omega;
			kinematics.position += n - rotate(n, omega * dt);
		} else {
			kinematics.position += dt * kinematics.velocity;
		}
		kinematics.orientation = sm::normalize_angle_absolute(
			kinematics.orientation + dt * kinematics.angular_velocity
		);
		
		for(std::size_t i = 0; i < config::Wheel::axis_angles.size(); ++i) {
			double v_weel = reference_velocities.local * polar(1.0, config::Wheel::axis_angles[i] + M_PI/2.0)
							+ kinematics.angular_velocity * config::Wheel::distance/2.0;
			;
			double omega_wheel = v_weel / config::Wheel::radius;
			kinematics.wheel_turn_angle[i] += omega_wheel*dt;
		}
		state_check(robot, "State_2");
	}
};

} /** namespace robo */
