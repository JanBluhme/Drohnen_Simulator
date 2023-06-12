#pragma once
#include "view/CoordinateSystemComponent.hpp"
#include "view/RobotHistoryMesh.hpp"
#include "view/EnvironmentViewBase.hpp"
#include "view/DebugLinesMesh.hpp"
#include "robo_commands.hpp"
#include "simple_gl/make_checker_board.hpp"
#include "simple_gl/make_simple_cone.hpp"
#include "simple_gl/Font.hpp"
#include "util/hsv_to_rgb.hpp"
#include "config/View.hpp"
#include "config/Wheel.hpp"
#include "config/Robot.hpp"
#include "config/Body.hpp"
#include "config/Textures.hpp"

namespace robo {

struct RobotsView {
	using mesh_data_t_wheel            = mesh_data_by_tags_t<has_normal_tag, has_tex_tag>;
	using mesh_data_t_robot_inner_body = mesh_data_by_tags_t<has_normal_tag, has_tex_tag>;
	using mesh_data_t_robot_outer_body = mesh_data_by_tags_t<has_normal_tag>;
	using gl_mesh_t_wheel              = mesh_data_t_wheel::gl_mesh_t;
	using gl_mesh_t_robot_inner_body   = mesh_data_t_robot_inner_body::gl_mesh_t;
	using gl_mesh_t_robot_outer_body   = mesh_data_t_robot_outer_body::gl_mesh_t;

	EnvironmentViewBase&                parent;
	Texture const                       texture_wheel;
	Texture const                       texture_body;
	Texture const                       texture_top;
	mesh_data_t_wheel            const  mesh_data_wheel;
	mesh_data_t_robot_inner_body const  mesh_data_robot_inner_body;
	mesh_data_t_robot_inner_body const  mesh_data_robot_top;
	mesh_data_t_robot_outer_body const  mesh_data_robot_outer_body;
	gl_mesh_t_wheel              const  mesh_wheel{           mesh_data_wheel.mesh(           &parent.window)};
	gl_mesh_t_robot_inner_body   const  mesh_robot_inner_body{mesh_data_robot_inner_body.mesh(&parent.window)};
	gl_mesh_t_robot_inner_body   const  mesh_robot_top{mesh_data_robot_top.mesh(&parent.window)};
	gl_mesh_t_robot_outer_body   const  mesh_robot_outer_body{mesh_data_robot_outer_body.mesh(&parent.window)};
	CoordinateSystemComponent const     coordinate_system{parent, 0.5, 0.05};

	enum class SelectMode {
		ALL, NONE, SELECTIVE
	};

	struct Features {
		bool show_label              = false;
		bool show_velocity           = false;
		bool show_reference_velocity = false;
		bool show_history            = true;
		bool show_debug_lines        = false;
		bool show_rays               = false;
		bool show_coordinate_system  = false;
	};

	Font                                font;
	SelectMode                          request_mode_labels               = SelectMode::SELECTIVE;
	SelectMode                          request_mode_velocities           = SelectMode::SELECTIVE;
	SelectMode                          request_mode_reference_velocities = SelectMode::SELECTIVE;
	SelectMode                          request_mode_histories            = SelectMode::SELECTIVE;
	SelectMode                          request_mode_debug_lines          = SelectMode::SELECTIVE;
	SelectMode                          request_mode_rays                 = SelectMode::SELECTIVE;
	SelectMode                          request_mode_coordinate_systems   = SelectMode::SELECTIVE;
	DebugLinesMesh                      debug_lines_mesh{&parent.window};
	std::vector<DebugLine>              velocities;
	DebugLinesMesh                      velocities_mesh{&parent.window};
	std::vector<DebugLine>              reference_velocities;
	DebugLinesMesh                      reference_velocities_mesh{&parent.window};
	std::vector<DebugLine>              rays;
	DebugLinesMesh                      rays_mesh{&parent.window};
	std::size_t                         history_size = 512;
	std::map<RobotId, RobotHistoryMesh> histories;
	std::map<RobotId, Features>         features;

	RobotsView(EnvironmentViewBase& parent)
		: parent{parent}
		, texture_wheel{make_checker_board(&parent.window, 2, 1,  128, 128, 255)}
		, texture_body{ &parent.window, config::Textures::robot3}
		, texture_top{  &parent.window, config::Textures::robot3}
		, mesh_data_wheel{           make_simple_cylinder<true,false>( config::Wheel::radius,            0.0,                   config::Wheel::thickness, 24, false, true)}
		, mesh_data_robot_inner_body{make_simple_cylinder<true,false>( config::Body::inner_radius,       config::Body::h0,      config::Body::h1       ,  24, false, false)}
		, mesh_data_robot_top{
			[]() {
				mesh_data_t_robot_inner_body result = make_simple_disk<true, false>(config::Body::inner_radius, 0.0, 2.0*M_PI, 24);
				for(auto& v : result.vertices) {
					v.tex[0] = (v.position[0] / config::Body::inner_radius + 1.0) / 2.0;
					v.tex[1] = (v.position[1] / config::Body::inner_radius + 1.0) / 2.0;
				}
				return Transform::translate(0.0, 0.0, config::Body::h1 * 0.98) * result;
			}()
		}
		, mesh_data_robot_outer_body{make_simple_cylinder<false,false>(config::Robot::radius,            config::Body::h0,      config::Body::h1,         24, false, true)}
		, font(&parent.window, config::View::label_font, config::View::label_font_size, config::View::label_height)
	{}

	static void toggle_select_mode(SelectMode& mode) {
		mode = mode == SelectMode::ALL
			? SelectMode::NONE
			: SelectMode::ALL
		;
	}
	
	void show_robot_wheels() {
		glEnable(GL_TEXTURE_2D);
		texture_wheel.bind();
		auto lock = mesh_wheel.lock();
		parent.set_color(Vertex<double,4>{1.0, 1.0, 1.0, 1.0});
		for(auto& robot : parent.sim_state.robots) {
			auto const& kinematics = robot.kinematics;
			scoped_draw([&]() {
				glTranslated(kinematics.position[0], kinematics.position[1], 0.0);
				glRotated(kinematics.orientation * sm::to_degrees, 0.0, 0.0, 1.0);
				for(std::size_t i = 0; i < config::Wheel::axis_angles.size(); ++i) {
					scoped_draw([&]() {
						glRotated(config::Wheel::axis_angles[i] * sm::to_degrees, 0.0, 0.0, 1.0);
						glTranslated(config::Wheel::distance/2 - config::Wheel::thickness/2, 0.0, config::Wheel::radius);
						glRotated(90.0, 1.0, 0.0, 0.0);
						glRotated(90.0, 0.0, 1.0, 0.0f);
						glRotated(-kinematics.wheel_turn_angle[i] * sm::to_degrees, 0.0, 0.0, 1.0);
						lock.draw();
					});
				}
			});
		}
		glDisable(GL_TEXTURE_2D);
	}
	
	Vertex<GLfloat,4> robot_color(Robot const& r, double v_scale = 1.0, double alpha = 1.0) const {
		double saturation = r.is_paused
			? 0.1
			: 0.7
		;
		if(r.killed) {
			saturation = 0.1;
			v_scale = 0.2;
		}
		Vertex<double,3> c = hsv_to_rgb(2*M_PI*5/6.0 * r.id.value / parent.sim_state.robots.size(), saturation, 0.8);
		return {
			  static_cast<GLfloat>(c[0] * v_scale)
			, static_cast<GLfloat>(c[1] * v_scale)
			, static_cast<GLfloat>(c[2] * v_scale)
			, static_cast<GLfloat>(alpha)
		};
	}
	
	void show_robot_inner_bodies() {
		glEnable(GL_TEXTURE_2D);
		texture_body.bind();
		auto lock = mesh_robot_inner_body.lock();
		for(auto& robot : parent.sim_state.robots) {
			auto const& kinematics = robot.kinematics;
			scoped_draw([&]() {
				glTranslated(kinematics.position[0], kinematics.position[1], 0.0);
				glRotated(kinematics.orientation * sm::to_degrees, 0.0, 0.0, 1.0);
				parent.set_color(robot_color(robot, 0.7));
				lock.draw();
			});
		}
		glDisable(GL_TEXTURE_2D);
	}
	void show_robot_tops() {
		glEnable(GL_TEXTURE_2D);
		texture_top.bind();
		auto lock = mesh_robot_top.lock();
		for(auto& robot : parent.sim_state.robots) {
			auto const& kinematics = robot.kinematics;
			scoped_draw([&]() {
				glTranslated(kinematics.position[0], kinematics.position[1], 0.0);
				glRotated(kinematics.orientation * sm::to_degrees, 0.0, 0.0, 1.0);
				parent.set_color(robot_color(robot, 0.7));
				lock.draw();
			});
		}
		glDisable(GL_TEXTURE_2D);
	}
	void show_robot_outer_bodies() {
		auto lock = mesh_robot_outer_body.lock();
		for(auto& robot : parent.sim_state.robots) {
			auto const& kinematics = robot.kinematics;
			scoped_draw([&]() {
				glTranslated(kinematics.position[0], kinematics.position[1], 0.0);
				if(!parent.is_simplified_render) {
					parent.set_color(robot_color(robot, 0.7, 0.4));
				} else {
					parent.set_color(robot_color(robot, 0.9, 1.0));
				}
				lock.draw();
			});
		}
	}
	void show_labels() {
		for(auto& robot : parent.sim_state.robots) {
			if(!features[robot.id].show_label) {
				continue;
			}
			parent.set_color(robot_color(robot));
			scoped_draw([&]() {
				glTranslated(
					  robot.kinematics.position[0] + config::Robot::radius
					, robot.kinematics.position[1]
					, 2*config::Body::h1
				);
				std::string id = std::to_string(robot.id.value);
				std::string score = std::to_string(robot.score);
				if(robot.taxi_guest) {
					score += " ("
						+ std::to_string(parent.sim_state.taxi_guests.guests[*robot.taxi_guest].score_on_arrival)
						+ ")"
					;
				}
				font.render(id + " -> " + score, 0, 0, 0);
			});
			if(!robot.name.empty()) {
				scoped_draw([&]() {
					glTranslated(
						  robot.kinematics.position[0] + config::Robot::radius
						, robot.kinematics.position[1] + config::View::label_height
						, 2*config::Body::h1
					);
					font.render(robot.name, 0, 0, 0);
				});
			}
		}
	}
	void show_robot_velocities() {
		velocities.clear();
		velocities.reserve(3*parent.sim_state.robots.size());
		Vertex<double, 3> red{  1.0, 0.0, 0.0};
		Vertex<double, 3> green{0.0, 1.0, 0.0};
		Vertex<double, 3> blue{ 0.0, 0.0, 1.0};
		double height = 0.2;
		double width  = 0.02;
		for(auto& robot : parent.sim_state.robots) {
			if(!features[robot.id].show_velocity) {
				continue;
			}
			auto const& kinematics = robot.kinematics;
			Vertex<double, 2> E_x = polar(1.0, kinematics.orientation);
			Vertex<double, 2> E_y = polar(1.0, kinematics.orientation + M_PI_2);
			Vertex<double,2> v_loc_x = E_x * (E_x * kinematics.velocity);
			Vertex<double,2> v_loc_y = E_y * (E_y * kinematics.velocity);
			
			double velocity_marker = kinematics.orientation
				+ M_PI * (kinematics.angular_velocity / config::Robot::angular_velocity_max)
			;
			Vertex<double,2> v_loc_z_start = polar(0.1 + config::Robot::radius, velocity_marker);
			Vertex<double,2> v_loc_z_end   = polar(0.4 + config::Robot::radius, velocity_marker);
			
			velocities.push_back(
				DebugLine::make(
					  kinematics.position
					, kinematics.position + v_loc_x
					, red
					, width
					, height
				)
			);
			velocities.push_back(
				DebugLine::make(
					  kinematics.position
					, kinematics.position + v_loc_y
					, green
					, width
					, height
				)
			);
			velocities.push_back(
				DebugLine::make(
					  kinematics.position
					, kinematics.position + v_loc_x + v_loc_y
					, Vertex<double,3>{1.0, 1.0, 1.0}
					, 0.5 * width
					, height
				)
			);
			velocities.push_back(
				DebugLine::make(
					  kinematics.position + v_loc_z_start
					, kinematics.position + v_loc_z_end
					, blue
					, width
					, height + 0.01
				)
			);
		}
		velocities_mesh.show(velocities.begin(), velocities.end());
	}
	void show_robot_reference_velocities() {
		reference_velocities.clear();
		reference_velocities.reserve(3*parent.sim_state.robots.size());
		Vertex<double, 3> red{  0.5, 0.2, 0.2};
		Vertex<double, 3> green{0.2, 0.5, 0.2};
		Vertex<double, 3> blue{ 0.2, 0.2, 0.5};
		double height = 0.1;
		double width  = 0.03;
		for(auto& robot : parent.sim_state.robots) {
			if(!features[robot.id].show_reference_velocity) {
				continue;
			}
			auto const& kinematics = robot.kinematics;
			using RSI_t = RobotStateIntegration<Environment::kinematic_model>;
			RobotVelocity ref = RSI_t::get_reference_velocities(robot, parent.environment.delta_t_simulation);
			double orientation = std::holds_alternative<LocalVelocityFixedFrameReference>(robot.reference)
				? std::get<LocalVelocityFixedFrameReference>(robot.reference).fixed_orientation
				: kinematics.orientation
			;
			Vertex<double,2> v_loc_x = rotate({ref.local[0], 0.0}, orientation);
			Vertex<double,2> v_loc_y = rotate({0.0, ref.local[1]}, orientation);

			double velocity_marker = orientation
				+ M_PI * (ref.angular_velocity / config::Robot::angular_velocity_max)
			;
			Vertex<double,2> v_loc_z_start = polar(0.1 + config::Robot::radius, velocity_marker);
			Vertex<double,2> v_loc_z_end   = polar(0.4 + config::Robot::radius, velocity_marker);

			reference_velocities.push_back(
				DebugLine::make(
					  kinematics.position
					, kinematics.position + v_loc_x
					, red
					, width
					, height
				)
			);
			reference_velocities.push_back(
				DebugLine::make(
					  kinematics.position
					, kinematics.position + v_loc_y
					, green
					, width
					, height
				)
			);
			reference_velocities.push_back(
				DebugLine::make(
					  kinematics.position
					, kinematics.position + v_loc_x + v_loc_y
					, Vertex<double,3>{0.5, 0.5, 0.5}
					, 0.5 * width
					, height
				)
			);
			reference_velocities.push_back(
				DebugLine::make(
					  kinematics.position + v_loc_z_start
					, kinematics.position + v_loc_z_end
					, blue
					, width
					, height + 0.01
				)
			);
		}
		reference_velocities_mesh.show(reference_velocities.begin(), reference_velocities.end());
	}
	void show_robot_rays() {
		rays.clear();
		rays.reserve(Robot::num_rays * parent.sim_state.robots.size());
		double height = 0.1;
		double width  = 0.02;
		for(auto& robot : parent.sim_state.robots) {
			if(!features[robot.id].show_rays) {
				continue;
			}
			auto const& kinematics = robot.kinematics;
			Vertex<float, 4> c = robot_color(robot);
			Vertex<double, 3> color{c[0], c[1], c[2]};
			for(std::size_t i = 0; i < Robot::num_rays; ++i) {
				double phi = i * 2.0 * M_PI / Robot::num_rays;
				Vertex<double, 2> E = polar(1.0, kinematics.orientation + phi);
				rays.push_back(
					DebugLine::make(
						  kinematics.position + E * config::Robot::radius
						, kinematics.position + E * robot.ray_distances[i]
						, color
						, width
						, height
					)
				);
			}
		}
		rays_mesh.show(rays.begin(), rays.end());
	}

	template<typename Map>
	void clean_map(Map& map) {
		auto const& robots = parent.sim_state.robots;
		for(auto it = map.begin(); it != map.end(); ) {
			auto r_it = std::find_if(
				  robots.begin()
				, robots.end()
				, [&](Robot const& r) {
					return r.id == it->first;
				}
			);
			if(r_it == robots.end()) {
				it = map.erase(it);
			} else {
				++it;
			}
		}
	}

	void show_histories() {
		auto const& robots = parent.sim_state.robots;
		for(auto& robot : robots) {
			auto it = histories.find(robot.id);
			if(it == histories.end()) {
				it = histories.emplace(
					  std::piecewise_construct
					, std::forward_as_tuple(robot.id)
					, std::forward_as_tuple(&parent.window)
				).first;
			}
			it->second.resize(history_size);
			auto color = robot_color(robot, 1.0, 0.5);
			it->second.update(robot, color);
			if(features[robot.id].show_history) {
// 				parent.set_color(color);
				it->second.show();
			}
		}
	}

	void show_debug_lines() {
		debug_lines_mesh.show(parent.sim_state.debug_lines.begin(), parent.sim_state.debug_lines.end());
	}

	auto debug_lines_selector() {
		return [&](RobotId id) {
			return features[id].show_debug_lines;
		};
	}

	void show_coordinate_systems() {
		auto const& robots = parent.sim_state.robots;
		for(auto& robot : parent.sim_state.robots) {
			if(!features[robot.id].show_coordinate_system) {
				continue;
			}
			auto const& kin = robot.kinematics;
			scoped_draw([&]() {
				glTranslated(kin.position[0], kin.position[1], 0.0);
				glRotated(kin.orientation * 180.0 / M_PI, 0.0, 0.0, 1.0);
				coordinate_system.show();
			});
		}
	}

	void show() {
		clean_map(histories);
		clean_map(features);
		show_coordinate_systems();
		show_robot_inner_bodies();
		show_robot_tops();
		show_robot_wheels();
		show_robot_outer_bodies();
		show_histories();
		show_robot_rays();
	}
	void show_overlays() {
		glDisable(GL_DEPTH_TEST);
		show_robot_reference_velocities();
		show_robot_velocities();
		show_debug_lines();
		show_labels();
		glEnable(GL_DEPTH_TEST);
	}
};

} /** namespace robo */
