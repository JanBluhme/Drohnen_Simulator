#pragma once
#include <cmath>

namespace sm {

inline double modulo(double a, double b) {
	return a - std::floor(a/b)*b;
}

inline double normalize_angle_absolute(double phi) {
	return modulo(phi, 2.0 * M_PI);
}

inline double normalize_angle_relative(double phi) {
	return normalize_angle_absolute(phi + M_PI) - M_PI;
}

constexpr double to_radians = M_PI / 180.0;
constexpr double to_degrees = 1.0 / to_radians;

} /** namespace sm */
