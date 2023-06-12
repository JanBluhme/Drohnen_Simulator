#pragma once
#include "config/Wheel.hpp"

namespace robo {
namespace config {

struct Body {
	constexpr static double inner_radius = 0.5*(Wheel::distance - Wheel::thickness/2);
	constexpr static double h0           = 0.003; // ground_clearance
	constexpr static double h1           = 3 * Wheel::radius;  // height
};

} /** namespace config */
} /** namespace robo */
