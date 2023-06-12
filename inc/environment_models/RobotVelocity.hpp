#pragma once
#include "math/Vertex.hpp"

struct RobotVelocity {
	Vertex<double,2> local {0.0, 0.0};
	Vertex<double,2> global{0.0, 0.0};
	double           angular_velocity{0.0};
};
