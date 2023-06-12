#pragma once
#include <vector>
#include <map>
#include <variant>
#include "simple_gl/GL_Window.hpp"
#include "simple_gl/Mesh.hpp"
#include "robo_commands.hpp"
#include "math/Vertex.hpp"
#include "config/Simulator.hpp"
#include "Environment.hpp"

namespace robo {

struct EnvironmentViewBase {
	Environment&            environment;
	GL_Window               window;
	bool                    is_simplified_render = false;
	bool                    is_show_coordinates = true;
	bool                    is_show_score  = false;
	bool                    is_show_labels = false;
	bool                    is_show_lines = true;
	bool                    is_show_logos  = true;
	bool                    is_show_remaining_time=false;
	Environment::GuiData    sim_state;

	EnvironmentViewBase(Environment& environment, int width, int height)
		: environment{environment}
		, window{
			  config::Simulator::name
			, width
			, height
		}
	{}

	template<typename debug_line_selector>
		requires std::is_invocable_r_v<bool, debug_line_selector, RobotId>
	void update(debug_line_selector selector) {
		sim_state = environment.get_gui_data(selector);
	}
	
	void set_color(Vertex<float,4> const& color) {
		if(is_simplified_render) {
			glColor4f(color[0], color[1], color[2], color[3]);
		} else {
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color.data());
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color.data());
		}
	}
	void set_color(Vertex<double,4> const& color) {
		set_color(
			Vertex<float,4>{
				  static_cast<float>(color[0])
				, static_cast<float>(color[1])
				, static_cast<float>(color[2])
				, static_cast<float>(color[3])
			}
		);
	}

	void setup_light() const {
		window.do_operation([&]() {
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			if(is_simplified_render) {
				glDisable(GL_LIGHT0);
				glDisable(GL_LIGHTING);
			} else {
				GLfloat light_diffuse[] = {0.9, 0.9, 0.9, 1.0};
				GLfloat light_position[] = {1.0, 1.0, 1.0, 1.0};
				glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
				glLightfv(GL_LIGHT0, GL_POSITION, light_position);
				glEnable(GL_LIGHT0);
				glEnable(GL_LIGHTING);
				GLfloat light[] = { 0.8, 0.8, 0.8, 1.0 };
				glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light);
				glEnable(GL_MULTISAMPLE);
				glEnable(GL_DEPTH_TEST);
			}
		});
	}
};

} /** namespace robo */
