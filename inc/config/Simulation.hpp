#pragma once
#include <algorithm>

namespace robo {
namespace config {

struct Simulation {
	constexpr static double delta_t_sim = 5e-3;
	constexpr static double gravity     = -9.81;
};

} // namespace config
} // namespace robo
