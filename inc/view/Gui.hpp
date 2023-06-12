#pragma once

#include <string>
#include "config/Hotkeys.hpp"
#include "config/Simulation.hpp"
#include "config/Build.hpp"
#include "view/Camera.hpp"
#include "view/ObstaclesView.hpp"
#include "view/RobotsView.hpp"
#include "view/SkyView.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl2.h"
#include "imgui/imgui_impl_sdl.h"
#include "view/EnvironmentViewBase.hpp"
#include <sstream>
#include <cstdio>

namespace robo {

namespace detail {

template<typename T, std::size_t N>
T from_buffer(std::array<char, N> const& src, T const& default_value) {
	std::istringstream ss{src.data()};
	T result;
	if(ss >> result) {
		return result;
	}
	return default_value;
}

template<typename T>
struct printer;

template<>
struct printer<double>
{
	constexpr static auto value = "%lf";
};

template<typename T, std::size_t N>
void to_buffer(T const& value, std::array<char, N>& dst) {
	snprintf(dst.data(), dst.size(), printer<T>::value, value);
}

} /* namespace detail */

struct Gui {
	constexpr static std::size_t buffer_size = 64;
	using hk            = robo::config::HotKeys;
	using text_buffer_t = std::array<char, buffer_size>;

	EnvironmentViewBase&       parent;
	Camera&                    camera;
	ObstaclesView&             obstacles;
	RobotsView&                robots;
	SkyView&                   sky;
	Vertex<double,3>           mouse_at;
	bool                       enable_sky_in_robot_cam     = true;
	bool                       enable_sky_in_non_robot_cam = false;
	text_buffer_t              populate_obstacles_N_str{'5','0',0};
	text_buffer_t              populate_guests_N_str{   '5','0',0};
	text_buffer_t              fps_simulation_str{'1', '0', 0};
	text_buffer_t              fps_vision_str{    '1', '0', 0};
	std::vector<text_buffer_t> add_obstacle_classes_N_str;

	Gui(EnvironmentViewBase& parent, Camera& camera, ObstaclesView& obstacles, RobotsView& robots, SkyView& sky)
		: parent{   parent   }
		, camera{   camera   }
		, obstacles{obstacles}
		, robots{   robots   }
		, sky{      sky      }
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::GetIO().IniFilename = nullptr;
		ImGui_ImplSDL2_InitForOpenGL(parent.window.window, parent.window.gl_context);
		ImGui_ImplOpenGL2_Init();
		detail::to_buffer(parent.environment.get_fps_simulation(), fps_simulation_str);
		detail::to_buffer(parent.environment.get_fps_vision()    , fps_vision_str    );
	}

	~Gui() {
		ImGui_ImplOpenGL2_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}

	auto hotkey(robo::config::HotKeys::HotKey const& hk)
		-> std::string
	{
		std::string ret{hk.name};
		ret += " (";
		ret += hk.key_name;
		ret += ")";
		return ret;
	}

	auto double_slider(
		  char const*  name
		, double&      value
		, double const min
		, double const max
		, char const*  format
		, auto...      as
	)
		-> bool
	{
		return ImGui::SliderScalar(name, ImGuiDataType_Double, &value, &min, &max, format, as...);
	};

	void show_mouse_at() {
		ImGui::Text("Global position: x = %.3fm, y = %.3fm", mouse_at[0], mouse_at[1]);
	}

	void show_simulation() {
		if(ImGui::TreeNode("Speed")) {
			if(ImGui::Checkbox(hotkey(hk::pause).c_str(), &parent.sim_state.is_paused)) {
				parent.environment.set_pause(parent.sim_state.is_paused);
			}
			if(ImGui::SliderInt("speed_scale", &parent.sim_state.speed_scale, -4, 8)) {
				parent.environment.set_speed_scale(parent.sim_state.speed_scale);
			}

			if(ImGui::InputText(
				  "fps_simulation"
				, fps_simulation_str.data()
				, fps_simulation_str.size()
				, ImGuiInputTextFlags_CharsDecimal
			)) {
				double fps = detail::from_buffer(fps_simulation_str, parent.environment.get_fps_simulation());
				if(fps >= 1.0) {
					parent.environment.set_fps_simulation(fps);
				} else {
					detail::to_buffer(1.0, fps_simulation_str);
				}
			}
			if(ImGui::InputText(
				  "fps_vision"
				, fps_vision_str.data()
				, fps_vision_str.size()
				, ImGuiInputTextFlags_CharsDecimal
			)) {
				double fps = detail::from_buffer(fps_vision_str, parent.environment.get_fps_vision());
				detail::from_buffer(fps_vision_str, fps);
				if(fps >= 1.0) {
					parent.environment.set_fps_vision(fps);
				} else {
					detail::to_buffer(1.0, fps_vision_str);
				}
			}
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("State")) {
			if(ImGui::Button(hotkey(hk::reset).c_str())) {
				parent.environment.reset();
			}
			bool akill = parent.environment.get_autokill_dead_robots();
			if(ImGui::Checkbox("autokill terminated robots", &akill)) {
				parent.environment.set_autokill_dead_robots(akill);
			}
			if(ImGui::BeginTable("table2", 3)) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				if(ImGui::Button("clear guests")) {
					parent.environment.clear_guests();
				}
				ImGui::TableSetColumnIndex(1);
				if(ImGui::Button("populate guests")) {
					parent.environment.populate_guests(detail::from_buffer<int>(populate_guests_N_str, 50));
				}
				ImGui::TableSetColumnIndex(2);
				ImGui::InputText(
					  "N_add##guests"
					, populate_guests_N_str.data()
					, populate_guests_N_str.size()
					, ImGuiInputTextFlags_CharsDecimal
				);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				if(ImGui::Button("clear obstacles")) {
					parent.environment.clear_obstacles();
				}
				ImGui::TableSetColumnIndex(1);
				if(ImGui::Button("populate obstacles")) {
					parent.environment.populate_obstacles(detail::from_buffer<int>(populate_obstacles_N_str, 50));
				}
				ImGui::TableSetColumnIndex(2);
				ImGui::InputText(
					  "N_add##obstacles"
					, populate_obstacles_N_str.data()
					, populate_obstacles_N_str.size()
					, ImGuiInputTextFlags_CharsDecimal
				);
				ImGui::EndTable();
			}
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("Manipulate Obstacles")) {
			ImGui::Checkbox("Movable obstacles", &parent.environment.obstacles.movable_obstacles);
			if(ImGui::TreeNode("Add Obstacles")) {
				if(ImGui::BeginTable("table_add_obs", 2)) {
					std::size_t N = parent.environment.obstacles.class_ids.size();
					add_obstacle_classes_N_str.reserve(N);
					for(std::size_t i = add_obstacle_classes_N_str.size(); i < N; ++i) {
						add_obstacle_classes_N_str.push_back(text_buffer_t{'1','0',0});
					}
					add_obstacle_classes_N_str.resize(N);
					for(std::size_t i = 0; i < parent.environment.obstacles.class_ids.size(); ++i) {
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::InputText(
							("N_add##obstacle" + std::to_string(i)).c_str()
							, add_obstacle_classes_N_str[i].data()
							, add_obstacle_classes_N_str[i].size()
							, ImGuiInputTextFlags_CharsDecimal
						);
						ImGui::TableSetColumnIndex(1);
						if(ImGui::Button(("add " + std::to_string(i)).c_str())) {
							std::size_t M = detail::from_buffer<int>(add_obstacle_classes_N_str[i], 10);
							for(std::size_t j = 0; j < M; ++j) {
								parent.environment.create_random_obstacle(parent.environment.obstacles.class_ids[i]);
							}
						}
					}
					ImGui::EndTable();
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
	}

	void show_camera() {
		if(ImGui::TreeNode("Mode")) {
			using State = Camera::State;
			State& state = camera.state;
			auto button = [&](std::string const& name, State match) {
				if(ImGui::RadioButton(name.c_str(), state == match)) {
					state = match;
				}
			};
			button(hotkey(hk::top_cam)                  , State::BIRDS_EYE                     );
			button("TopCamPan"                          , State::BIRDS_EYE_PAN                 );
			button(hotkey(hk::bot_cam_fixed_orientation), State::FOLLOW_ROBOT_FIXED_ORIENTATION);
			button(hotkey(hk::bot_cam)                  , State::FOLLOW_ROBOT                  );
			button("BotCamR"                            , State::FOLLOW_ROBOT_ROTATING         );
			button(hotkey(hk::bot_cam_pan)              , State::BIRDS_EYE_PAN_ROBOT           );
			button(hotkey(hk::user_cam)                 , State::USER                          );
			if(camera.state == State::FOLLOW_ROBOT_ROTATING) {
				double scale = 100.0;
				double max   = 0.1;
				double x = scale * camera.dt;
				if(double_slider("omega", x, -max * scale, max * scale, "%.2f")) {
					camera.dt = x / scale;
				}
			}
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("Tracked Robot")) {
			for(auto const& robot: parent.sim_state.robots) {
				bool active = camera.tracked_robot == robot.id;
				if(ImGui::RadioButton(std::to_string(robot.id.value).c_str(), active)) {
					camera.tracked_robot = robot.id;
				}
				if(&robot != &parent.sim_state.robots.back()) {
					ImGui::SameLine();
				}
			}
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("Movement")) {
			double_slider("tau_camera", camera.camera_pose.tau                     , 0.0000001, 10.0, "%.2f");
			double_slider("tau_robot" , camera.local_robot_sluggish_camera_pose.tau, 0.0000001, 10.0, "%.2f");
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("Projection")) {
			bool has_changed = false;
			if(ImGui::Checkbox("ortho", &camera.is_ortho)) {
				has_changed = true;
			}
			if(camera.is_ortho) {
				bool is_birds_eye =
					   camera.state == Camera::State::BIRDS_EYE
					|| camera.state == Camera::State::BIRDS_EYE_PAN
					|| camera.state == Camera::State::BIRDS_EYE_PAN_ROBOT
				;
				if(!is_birds_eye && double_slider("ortho_height", camera.w_height, 0.1 , 30.0 , "%.2f")) {
					has_changed = true;
				}

				if(    double_slider("ortho_z_near" , camera.ortho_z_near, -100.0, 0.9 * camera.ortho_z_far, "%.2f m")
					|| double_slider("ortho_z_far"  , camera.ortho_z_far , 1.1 * camera.ortho_z_near, 100.0, "%.2f m")
				) {
					has_changed = true;
				}
			} else {
				if(    double_slider("fov"   , camera.field_of_view_degrees, 1.00, 179.0, "%.0f degrees")
					|| double_slider("z_near", camera.perspective_z_near   , 0.01, 0.9 * camera.perspective_z_far, "%.2f m")
					|| double_slider("z_far" , camera.perspective_z_far    , 1.1 * camera.perspective_z_near, 100.0, "%.2f m")
				) {
					has_changed = true;
				}
			}
			if(has_changed) {
				camera.fix_projection();
			}
			ImGui::TreePop();
		}
	}

	void show_robot_features() {
		int hist_size = static_cast<int>(robots.history_size);
		if(ImGui::SliderInt("history size", &hist_size, 1, 2048)) {
			robots.history_size = static_cast<std::size_t>(hist_size);
		}

		using Mode = RobotsView::SelectMode;
		auto button = [&](std::string const& name, Mode& source_sink, Mode match) {
			if(ImGui::RadioButton(name.c_str(), source_sink == match)) {
				source_sink = match;
			}
		};
		// id, labels, velocities, histories, debug_lines
		if(ImGui::BeginTable("table2", 9)) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("robot");
			ImGui::Text("");

			auto combo = [](std::string const& name, Mode& mode) {
				const char* items[] = { "ALL", "NONE", "SELECT"};
				int item_current = mode == Mode::ALL
					? 0
					: mode == Mode::NONE
						? 1
						: 2
				;
				ImGui::Text("%s",name.c_str());
				ImGui::Combo(("      ##"+name).c_str(), &item_current, items, IM_ARRAYSIZE(items));
				mode = item_current == 0
					? Mode::ALL
					: item_current == 1
						? Mode::NONE
						: Mode::SELECTIVE
				;
			};

			ImGui::TableSetColumnIndex(1);
			combo("labels", robots.request_mode_labels);
			ImGui::TableSetColumnIndex(2);
			combo("coordinate_systems", robots.request_mode_coordinate_systems);
			ImGui::TableSetColumnIndex(3);
			combo("velocities", robots.request_mode_velocities);
			ImGui::TableSetColumnIndex(4);
			combo("reference_velocities", robots.request_mode_reference_velocities);
			ImGui::TableSetColumnIndex(5);
			combo("rays", robots.request_mode_rays);
			ImGui::TableSetColumnIndex(6);
			combo("histories", robots.request_mode_histories);
			ImGui::TableSetColumnIndex(7);
			combo("debug_lines", robots.request_mode_debug_lines);

			if(    robots.request_mode_labels               == Mode::SELECTIVE
				|| robots.request_mode_velocities           == Mode::SELECTIVE
				|| robots.request_mode_reference_velocities == Mode::SELECTIVE
				|| robots.request_mode_rays                 == Mode::SELECTIVE
				|| robots.request_mode_histories            == Mode::SELECTIVE
				|| robots.request_mode_debug_lines          == Mode::SELECTIVE
			) {
				for(auto& [id, features] : robots.features) {
					ImGui::TableNextRow();
					std::string id_str = std::to_string(id.value);
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%s", id_str.c_str());
					if(robots.request_mode_labels == Mode::SELECTIVE) {
						ImGui::TableSetColumnIndex(1);
						ImGui::Checkbox(("##label_" + id_str).c_str(), &features.show_label);
					}
					if(robots.request_mode_coordinate_systems == Mode::SELECTIVE) {
						ImGui::TableSetColumnIndex(2);
						ImGui::Checkbox(("##cs_" + id_str).c_str(), &features.show_coordinate_system);
					}
					if(robots.request_mode_velocities == Mode::SELECTIVE) {
						ImGui::TableSetColumnIndex(3);
						ImGui::Checkbox(("##velocity" + id_str).c_str(), &features.show_velocity);
					}
					if(robots.request_mode_reference_velocities == Mode::SELECTIVE) {
						ImGui::TableSetColumnIndex(4);
						ImGui::Checkbox(("##ref_velocity" + id_str).c_str(), &features.show_reference_velocity);
					}
					if(robots.request_mode_rays == Mode::SELECTIVE) {
						ImGui::TableSetColumnIndex(5);
						ImGui::Checkbox(("##rays" + id_str).c_str(), &features.show_rays);
					}
					if(robots.request_mode_histories == Mode::SELECTIVE) {
						ImGui::TableSetColumnIndex(6);
						ImGui::Checkbox(("##history" + id_str).c_str(), &features.show_history);
					}
					if(robots.request_mode_debug_lines == Mode::SELECTIVE) {
						ImGui::TableSetColumnIndex(7);
						ImGui::Checkbox(("##debug_lines" + id_str).c_str(), &features.show_debug_lines);
					}
					ImGui::TableSetColumnIndex(8);
					const char* items[] = { "LEAVE", "ALL", "NONE"};
					int item_current = 0;
					if(ImGui::Combo(("all_none##_" + id_str).c_str(), &item_current, items, IM_ARRAYSIZE(items))) {
						auto const enable_disable = [&](RobotsView::Features& f, bool value) {
							if(robots.request_mode_labels == Mode::SELECTIVE) {
								f.show_label = value;
							}
							if(robots.request_mode_coordinate_systems == Mode::SELECTIVE) {
								f.show_coordinate_system = value;
							}
							if(robots.request_mode_velocities == Mode::SELECTIVE) {
								f.show_velocity = value;
							}
							if(robots.request_mode_reference_velocities == Mode::SELECTIVE) {
								f.show_reference_velocity = value;
							}
							if(robots.request_mode_rays == Mode::SELECTIVE) {
								f.show_rays = value;
							}
							if(robots.request_mode_histories == Mode::SELECTIVE) {
								f.show_history = value;
							}
							if(robots.request_mode_debug_lines == Mode::SELECTIVE) {
								f.show_debug_lines = value;
							}
						};
						if(item_current == 1) {
							enable_disable(features, true);
						}
						if(item_current == 2) {
							enable_disable(features, false);
						}
					}
				}
			}
			ImGui::EndTable();
		}
	}
	
	void update_robot_features_all_none() {
		using Mode = RobotsView::SelectMode;
		for(auto& [id, features] : robots.features) {
			auto all_none = [](Mode mode, bool& sink) {
				if(mode == Mode::ALL) {
					sink = true;
				}
				if(mode == Mode::NONE) {
					sink = false;
				}
			};
			all_none(robots.request_mode_coordinate_systems  , features.show_coordinate_system );
			all_none(robots.request_mode_labels              , features.show_label             );
			all_none(robots.request_mode_velocities          , features.show_velocity          );
			all_none(robots.request_mode_reference_velocities, features.show_reference_velocity);
			all_none(robots.request_mode_rays                , features.show_rays              );
			all_none(robots.request_mode_histories           , features.show_history           );
			all_none(robots.request_mode_debug_lines         , features.show_debug_lines       );
		}
	}

	void show_obstacles() {
		if(ImGui::BeginTable("table1", 3)) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Checkbox("obstacle", &obstacles.show_original);
			if(obstacles.show_original) {
				ImGui::TableSetColumnIndex(1);
				ImGui::Checkbox("texture", &obstacles.show_texture);
				ImGui::TableSetColumnIndex(2);
				double_slider("alpha_o", obstacles.alpha_original    , 0.0, 1.0, "%.2f");
			}

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Checkbox("collision"   , &obstacles.show_collision);
			if(obstacles.show_collision) {
				ImGui::TableSetColumnIndex(2);
				double_slider("alpha_c", obstacles.alpha_collision   , 0.0, 1.0, "%.2f");
			}

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Checkbox("bounding_box", &obstacles.show_bounding_box);
			if(obstacles.show_bounding_box) {
				ImGui::TableSetColumnIndex(2);
				double_slider("alpha_b", obstacles.alpha_bounding_box, 0.0, 1.0, "%.2f");
			}
			ImGui::EndTable();
		}
	}

	void process_enable_sky() {
		bool is_robot_cam = camera.current_camera() == &camera.local_robot_camera;
		if(is_robot_cam) {
			sky.enabled = enable_sky_in_robot_cam;
		} else {
			sky.enabled = enable_sky_in_non_robot_cam;
		}
	}

	void show_features() {
		if(ImGui::TreeNode("Robot")) {
			show_robot_features();
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("Obstacles")) {
			show_obstacles();
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("Sky")) {
			ImGui::Checkbox("sky_in_robot_cam", &enable_sky_in_robot_cam);
			ImGui::Checkbox("sky_in_non_robot_cam", &enable_sky_in_non_robot_cam);
			if(sky.enabled) {
				if(ImGui::Button("next_texture ")) {
					sky.next_texture();
				}
			}
			ImGui::TreePop();
		}
	}

	void show_help() {
		if(ImGui::TreeNode("Camera")) {
			if(ImGui::BeginTable("table_cam", 3)) {
				char const* mesg[][2] {
					  {"CTRL + MouseMotion_x" , "Azimuth"         }
					, {"CTRL + MouseMotion_y" , "Elevation"       }
					, {"SHIFT + MouseMotion_x", "Left/Right"      }
					, {"SHIFT + MouseMotion_y", "Forward/Backward"}
					, {"MouseWheel"           , "Distance"        }
				};
				for(auto const m : mesg) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%s", m[0]);
					ImGui::TableSetColumnIndex(1);
					ImGui::Text(":");
					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%s", m[1]);
				}
				ImGui::EndTable();
			}
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("Hotkeys")) {
			if(ImGui::BeginTable("table_hotkeys", 3)) {
				for(auto const& hk : config::HotKeys::hotkeys) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%s", std::string{hk.name}.c_str());
					ImGui::TableSetColumnIndex(1);
					ImGui::Text(":");
					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%s", std::string{hk.key_name}.c_str());
				}
				ImGui::EndTable();
			}
			ImGui::TreePop();
		}
	}

	void show_about() {
		if(ImGui::TreeNode("Help")) {
			show_help();
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("Authors")) {
			ImGui::Text("Norbert Schmitz");
			ImGui::Text("Dominic Pöschko");
			ImGui::Text("Christoph Ußfeller");
			ImGui::TreePop();
		}
		if(ImGui::TreeNode("Build")) {
			ImGui::Text("%s, %s", config::Build::date, config::Build::time);
			ImGui::Text("git#: %s", config::Build::git);
			ImGui::TreePop();
		}
	}

	void show() {
		update_robot_features_all_none();
		process_enable_sky();
		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplSDL2_NewFrame(parent.window.window);
		ImGui::NewFrame();

		ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);
		ImGui::SetNextWindowPos(ImVec2{0.0, 0.0}, ImGuiCond_Once);
		ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		if(!ImGui::IsWindowCollapsed()) {
			auto may_show = [&](char const* name, auto op) {
				if(ImGui::CollapsingHeader(name)) {
					op();
				}
			};
			may_show("Mouse"               , [&](){show_mouse_at();            });
			may_show("Simulation"          , [&](){show_simulation();          });
			may_show("Camera"              , [&](){show_camera();              });
			may_show("Features"            , [&](){show_features();            });
			may_show("About"               , [&](){show_about();               });
		}
		ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
	}
};
}   // namespace robo
