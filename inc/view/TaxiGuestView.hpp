#pragma once
#include "simple_gl/make_simple_cone.hpp"
#include "simple_gl/make_checker_board.hpp"
#include "simple_gl/Texture.hpp"
#include "math/PointLine.hpp"
#include "config/View.hpp"
#include <vector>
#include "view/EnvironmentViewBase.hpp"
#include "view/RobotsView.hpp"
#include "util/make_array.hpp"
#include "config/Textures.hpp"

namespace robo {

struct TaxiGuestView {
	using mesh_data_t = mesh_data_by_tags_t<has_normal_tag, has_tex_tag>;
	using gl_mesh_t   = mesh_data_t::gl_mesh_t;



	EnvironmentViewBase& parent;
	RobotsView const&    robots_view;
	Texture const        texture_body_inactive;
	Texture const        texture_body_active;
	Texture const        texture_body_target;
	Texture const        texture_legs_inactive;
	Texture const        texture_legs_active;
	Texture const        texture_legs_target;
	Texture const        texture_ears_inactive;
	Texture const        texture_ears_active;
	Texture const        texture_ears_target;
	mesh_data_t const    mesh_data_body;
	mesh_data_t const    mesh_data_legs;
	mesh_data_t const    mesh_data_ears;
	gl_mesh_t   const    mesh_body{mesh_data_body.mesh(&parent.window)};
	gl_mesh_t   const    mesh_legs{mesh_data_legs.mesh(&parent.window)};
	gl_mesh_t   const    mesh_ears{mesh_data_ears.mesh(&parent.window)};

	static auto make_mesh_data_body(double radius)
		-> mesh_data_t
	{
		return
			  Transform::translate(0.0, 0.0, 2.0 * radius)
			* Transform::rotate_y_pi_halfs<2>()
			* make_simple_sherical_segment<true, false>(
					radius
				, -M_PI_2
				,  M_PI_2
				,  0.0
				,  2.0 * M_PI
				,  12
				,  12
			)
		;
	}
	static auto make_mesh_data_legs(double radius)
		-> mesh_data_t
	{
		mesh_data_t cone = make_simple_cone_segment<true, false>(
			   0.3 * radius
			, -radius
			,  0.1 * radius
			, -2.0 * radius
			,  0.0
			,  2.0 * M_PI
			,  3
			,  false
			,  false
		);
		cone = Transform::rotate_x(1.0 * M_PI / 6.0) * cone;
		return
			  Transform::translate(0.0, 0.0, 2.0 * radius)
			* ( Transform::rotate_z(0.0 * M_PI / 3.0) * cone
				+ Transform::rotate_z(2.0 * M_PI / 3.0) * cone
				+ Transform::rotate_z(4.0 * M_PI / 3.0) * cone
			)
		;
	}
	static auto make_mesh_data_ears(double radius)
		-> mesh_data_t
	{
		mesh_data_t sphere = make_simple_sherical_segment<true, false>(
			  radius / 3.0
			, -M_PI_2
			,  M_PI_2
			,  0.0
			,  2.0 * M_PI
			,  12
			,  12
		);

		return
			  Transform::translate(0.0, 0.0, 2.0 * radius)
			* (   Transform::translate(0.0, -radius/1.5, 0.8*radius) * sphere
				+ Transform::translate(0.0,  radius/1.5, 0.8*radius) * sphere
			)
		;
	}


	TaxiGuestView(EnvironmentViewBase& parent, RobotsView const& robots_view)
		: parent{parent}
		, robots_view{robots_view}
		, texture_body_inactive{
			&parent.window, config::Textures::smiley_neptune
		}
		, texture_body_active{
			&parent.window, config::Textures::smiley_neptune_gray
		}
		, texture_body_target{
			make_checker_board(&parent.window, 20, 20, 1200, 800,  255)
		}
		, texture_legs_inactive{
			&parent.window, config::Textures::X_2k_neptune
		}
		, texture_legs_active{
			&parent.window, config::Textures::panel3
		}
		, texture_legs_target{
			make_checker_board(&parent.window, 20, 20, 1200, 800,  255)
		}
		, texture_ears_inactive{
			&parent.window, config::Textures::X_2k_neptune
		}
		, texture_ears_active{
			&parent.window, config::Textures::panel3
		}
		, texture_ears_target{
			make_checker_board(&parent.window, 20, 20, 1200, 800,  255)
		}
		, mesh_data_body{make_mesh_data_body(0.05)}
		, mesh_data_legs{make_mesh_data_legs(0.05)}
		, mesh_data_ears{make_mesh_data_ears(0.05)}
	{}

	void show() {
		auto show_component = [](
			  Vertex<double,2> const& position
			, double                  height
			, double                  orientation
			, auto&                   mesh_lock
		) {
			scoped_draw([&]() {
				glTranslated(position[0], position[1], height);
				glRotated(orientation * 180.0 / M_PI, 0.0, 0.0, 1.0);
				mesh_lock.draw();
			});
		};
		auto show_components = [&](
			  gl_mesh_t const& mesh
			, Texture const&   texture_inactive
			, Texture const&   texture_active
			, Texture const&   texture_target
		) {

			{
				texture_inactive.bind();
				auto lock = mesh.lock();
				parent.set_color(Vertex<double,4>{1.0,1.0,1.0,1.0});
				for(auto const& g: parent.sim_state.taxi_guests.guests) {
					if(!g.bound_to_robot && !g.done) {
						show_component(g.position, g.height, g.rotation, lock);
					}
				}
			}
			{
				texture_active.bind();
				auto lock = mesh.lock();
				for(auto const& g: parent.sim_state.taxi_guests.guests) {
					if(g.bound_to_robot && !g.done) {
						Robot const* robot = parent.environment.robots.find(*g.bound_to_robot);
						if(robot) {
							parent.set_color(robots_view.robot_color(*robot, 0.7));
						} else {
							parent.set_color(Vertex<double,4>{1.0,1.0,1.0,1.0});
						}
						show_component(g.position, g.height, g.rotation, lock);
					}
				}
			}
			{
				texture_target.bind();
				auto lock = mesh.lock();
				for(auto const& g: parent.sim_state.taxi_guests.guests) {
					if(g.bound_to_robot && !g.done) {
						Robot const* robot = parent.environment.robots.find(*g.bound_to_robot);
						if(robot) {
							parent.set_color(robots_view.robot_color(*robot, 0.7));
						} else {
							parent.set_color(Vertex<double,4>{1.0,1.0,1.0,1.0});
						}
						show_component(g.target_position, g.height, g.rotation, lock);
					}
				}
			}
		};
		glEnable(GL_TEXTURE_2D);
		show_components(mesh_body, texture_body_inactive, texture_body_active, texture_body_target);
		show_components(mesh_legs, texture_legs_inactive, texture_legs_active, texture_legs_target);
		show_components(mesh_ears, texture_ears_inactive, texture_ears_active, texture_ears_target);
		glDisable(GL_TEXTURE_2D);
	}
};

} /** namespace robo */
