#pragma once
#include "math/Vertex.hpp"
#include "math/OrthonormalBasis.hpp"
#include "Robot.hpp"
#include "config/Robot.hpp"
#include "config/Body.hpp"

namespace robo {

struct CollisionModel {
	static bool collide(
		  Vertex<double,2>& position_a
		, Vertex<double,2>& velocity_a
		, double radius_a
		, double mass_a
		, double height_a
		, Vertex<double,3>& position3_b
		, Vertex<double,3>& velocity3_b
		, double radius_b
		, double mass_b
	) {
		if(position3_b[2] > height_a) {
			return false;
		}
		Vertex<double,2> position_b{position3_b[0], position3_b[1]};
		Vertex<double,2> velocity_b{velocity3_b[0], velocity3_b[1]};
		Vertex<double,2> n = position_b - position_a;
		double dist = n.length();
		if(dist < 1e-6) {
			// can't transfrom_velocity...
			// so don't change anything, but report collision...
			return true;
		}
		if(dist > radius_a + radius_b) {
			return false;
		}
		sm::OrthonormalBasis transfrom_velocity(n);
		Vertex<double,2> val = transfrom_velocity.to_local(velocity_a);
		Vertex<double,2> vbl = transfrom_velocity.to_local(velocity_b);
		double va1x = val[0];
		double vb1x = vbl[0];
		val[0] = ((mass_a - mass_b) * va1x + 2.0 * mass_b * vb1x) / (mass_a + mass_b);
		vbl[0] = ((mass_b - mass_a) * vb1x + 2.0 * mass_a * va1x) / (mass_a + mass_b);
		velocity_a = transfrom_velocity.to_global(val);
		velocity_b = transfrom_velocity.to_global(vbl);
		double dist_a = (dist*dist + radius_a * radius_a - radius_b * radius_b) / (2.0 * dist);
		if(!std::isnormal(dist_a)) {
			dist_a = (radius_a + radius_b)/2;
		}
		dist_a = std::clamp(dist_a, 1e-3*radius_a, radius_a);
		Vertex<double,2> c = position_a + dist_a * transfrom_velocity.Ex;
		position_a = c - radius_a * transfrom_velocity.Ex;
		position_b = c + radius_b * transfrom_velocity.Ex;

		position3_b[0] = position_b[0];
		position3_b[1] = position_b[1];
		velocity3_b[0] = velocity_b[0];
		velocity3_b[1] = velocity_b[1];
		
		return true;
	}
	
	static bool collide(
		  Vertex<double,2>& position_a
		, Vertex<double,2>& velocity_a
		, double radius_a
		, double mass_a
		, Vertex<double,2>& position_b
		, Vertex<double,2>& velocity_b
		, double radius_b
		, double mass_b
	) {
		Vertex<double, 3> pos_b {position_b[0], position_b[1], 0.0};
		Vertex<double, 3> vel_b {velocity_b[0], velocity_b[1], 0.0};
		bool result = collide(
			  position_a
			, velocity_a
			, radius_a
			, mass_a
			, 1.0
			, pos_b
			, vel_b
			, radius_b
			, mass_b
		);
		position_b[0] = pos_b[0];
		position_b[1] = pos_b[1];
		velocity_b[0] = vel_b[0];
		velocity_b[1] = vel_b[1];
		return result;
	}
	
	static bool collide(Robot& robot_a, Robot& robot_b) {
		return collide(
			  robot_a.kinematics.position
			, robot_a.kinematics.velocity
			, config::Robot::radius
			, config::Robot::mass
			, robot_b.kinematics.position
			, robot_b.kinematics.velocity
			, config::Robot::radius
			, config::Robot::mass
		);
	}
};

} /** namespace robo */
