#pragma once

#include <SDL2/SDL.h>
#include <string_view>
#include "util/make_array.hpp"


namespace robo {
namespace config {

struct HotKeys {
	struct HotKey {
		std::string_view name;
		std::string_view key_name;
		int              value;
	};

	static constexpr HotKey pause{                          "Pause"                    , "<space>", ' '      };
	static constexpr HotKey reset{                          "Reset"                    , "<F8>"   , SDLK_F8  };
	static constexpr HotKey next_cam{                       "NextCam"                  , "c"      , 'c'      };
	static constexpr HotKey top_cam{                        "TopCam"                   , "x"      , 'x'      };
	static constexpr HotKey bot_cam{                        "BotCam"                   , "y"      , 'y'      };
	static constexpr HotKey bot_cam_fixed_orientation{      "BotCam_FO"                , "a"      , 'a'      };
	static constexpr HotKey bot_cam_pan{                    "BotCam_Pan"               , "z"      , 'z'      };
	static constexpr HotKey user_cam{                       "UserCam"                  , "u"      , 'u'      };
	static constexpr HotKey speed_up{                       "Speedup"                  , "<UP>"   , SDLK_UP  };
	static constexpr HotKey speed_down{                     "Slowdown"                 , "<DOWN>" , SDLK_DOWN};
	static constexpr HotKey toggle_all_labels{              "ToggleLabels"             , "l"      , 'l'      };
	static constexpr HotKey toggle_all_velocities{          "ToggleVelocities"         , "v"      , 'v'      };
	static constexpr HotKey toggle_all_reference_velocities{"ToggleReferenceVelocities", "w"      , 'w'      };
	static constexpr HotKey toggle_all_rays{                "ToggleRays"               , "r"      , 'r'      };
	static constexpr HotKey toggle_all_histories{           "ToggleHistories"          , "h"      , 'h'      };
	static constexpr HotKey toggle_all_debug_lines{         "ToggleDebugLines"         , "d"      , 'd'      };
	static constexpr HotKey toggle_all_coordinate_systems{  "ToggleCoordinateSystems"  , "s"      , 's'      };
	static constexpr HotKey reset_robot_cam_center{         "ResetRobotCamCenter"      , "<"      , '<'      };

	static constexpr auto hotkeys = make_array<HotKey>(
		  pause
		, reset
		, next_cam
		, top_cam
		, bot_cam
		, bot_cam_fixed_orientation
		, bot_cam_pan
		, user_cam
		, speed_up
		, speed_down
		, toggle_all_labels
		, toggle_all_velocities
		, toggle_all_reference_velocities
		, toggle_all_rays
		, toggle_all_histories
		, toggle_all_debug_lines
		, toggle_all_coordinate_systems
		, reset_robot_cam_center
	);
};

} // namespace config
} // namespace robo
