#pragma once

#include "util/time_this.hpp"
#include "view/Camera.hpp"
#include "view/CoordinateSystemView.hpp"
#include "view/EnvironmentViewBase.hpp"
#include "view/PitchView.hpp"
#include "view/RobotsView.hpp"
#include "view/Gui.hpp"
#include "view/ObstaclesView.hpp"
#include "view/TaxiGuestView.hpp"
#include "view/SkyView.hpp"
#include "view/CameraBasePositionView.hpp"

namespace robo {

struct EnvironmentView
	: EnvironmentViewBase
{
	Camera                 camera;
	CoordinateSystemView   coordinate_system_view;
	RobotsView             robots_view;
	PitchView              pitch_view;
	ObstaclesView          obstacles_view;
	TaxiGuestView          taxi_guest_view;
	SkyView                sky_view;
	CameraBasePositionView camera_base_position_view;
	std::optional<Gui>     gui;

	EnvironmentView(Environment& environment, int width, int height, bool has_imgui)
		: EnvironmentViewBase{environment, width, height}
		, camera{this->window, width, height}
		, coordinate_system_view{   *this}
		, robots_view{              *this}
		, pitch_view{               *this}
		, obstacles_view{           *this}
		, taxi_guest_view{          *this, robots_view}
		, sky_view{                 *this}
		, camera_base_position_view{*this, camera, robots_view}
		, gui{has_imgui
			? std::optional<Gui>{std::in_place_t{}, *this, camera, obstacles_view, robots_view, sky_view}
			: std::optional<Gui>{}
		}
	{}

	void render(bool show_camera_foot) {
		time_this("render", [&]() {
			time_this("render::copy_state", [&]() { this->update(robots_view.debug_lines_selector()); });
			this->window.draw([&]() {
				time_this("render::setup_camera", [&]() {
					camera.setup(this->sim_state.robots);
					camera.update_transform();
				});
				time_this("render::setup_light"                   , [&]() { this->setup_light();          });
				time_this("render::show_plane"                    , [&]() { pitch_view.show();            });
				time_this("render::show_coordinate_system"        , [&]() { coordinate_system_view.show();});
				time_this("render::show_camera_base_position_view", [&]() { camera_base_position_view.show(show_camera_foot);});
				time_this("render::show_taxi_guests"              , [&]() { taxi_guest_view.show();       });
				time_this("render::show_robots"                   , [&]() { robots_view.show();           });
				time_this("render::show_obstacles"                , [&]() { obstacles_view.show();        });
				time_this("render::show_sky"                      , [&]() { sky_view.show();              });
				time_this("render::show_robots_overlays"          , [&]() { robots_view.show_overlays();  });
				if(gui) {
					time_this("render::gui", [&]() { gui->show(); });
				}
			});
		});
	}
};

}   // namespace robo
