#pragma once
#include "math/Vertex.hpp"
#include "math/angle_util.hpp"

namespace robo {
namespace config {

struct Wheel {
	constexpr static Vertex<double, 4> axis_angles {
		   -70.0 * sm::to_radians
		,   70.0 * sm::to_radians
		, -135.0 * sm::to_radians
		,  135.0 * sm::to_radians
	};
	constexpr static double radius    = 0.04;
	constexpr static double distance  = 0.2;
	constexpr static double thickness = 0.015;
};

} /** namespace config */
} /** namespace robo */
