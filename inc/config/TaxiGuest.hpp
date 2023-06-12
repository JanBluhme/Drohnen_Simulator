#pragma once

namespace robo {
namespace config {

struct TaxiGuest {
	constexpr static double max_pick_distance       =  0.3;
	constexpr static double max_drop_distance       =  0.3;
	constexpr static double max_visibility_distance =  2.0;
	constexpr static double max_pick_velocity       =  0.01;
	constexpr static double max_drop_velocity       =  0.01;
	constexpr static double min_tip                 =  0.0;
	constexpr static double max_tip                 =  2.0;
	constexpr static double points_per_meter        = 10.0;
};

} /** namespace config */
} /** namespace robo */
