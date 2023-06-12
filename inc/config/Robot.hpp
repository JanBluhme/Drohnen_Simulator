#pragma once
#include <cmath>
#include "config/Wheel.hpp"

namespace robo {
namespace config {

struct Robot {
	constexpr static double velocity_max_x           = 1.7;
	constexpr static double velocity_max_y           = 1.0;
	constexpr static double acceleration_max_x       = 7.0;
	constexpr static double acceleration_max_y       = 5.0;
	constexpr static double acceleration_max_angular = 5.0 * 2.0 * M_PI;
	constexpr static double angular_velocity_max     = 4.0 * 2.0 * M_PI;
	constexpr static double mass                     = 4.00;
	constexpr static double radius = 0.5*(Wheel::distance - Wheel::thickness/2) + 1.3*Wheel::thickness;
	constexpr static double max_visibility_distance =  2.0;
};

} /** namespace config */
} /** namespace robo */
