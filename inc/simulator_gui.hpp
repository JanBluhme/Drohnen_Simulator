#pragma once
#include "EnvironmentView.hpp"
#include "config/Hotkeys.hpp"
#include "util/name_this_thread.hpp"
#include <atomic>
#include <thread>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "Environment.hpp"

inline bool handle_key(
	  SDL_KeyboardEvent& event
	, robo::EnvironmentView* view
) {
	using hk = robo::config::HotKeys;
	switch(event.keysym.sym) {
		case 27: {
			return false;
		}
		case hk::pause.value: {
			view->environment.toggle_pause();
			break;
		}
		case hk::reset.value: {
			view->environment.reset();
			break;
		}
		case hk::next_cam.value: {
			view->camera.next();
			break;
		}
		case hk::bot_cam_pan.value: {
			if(view->camera.state == robo::Camera::State::BIRDS_EYE_PAN_ROBOT) {
				view->camera.next_robot(view->sim_state.robots);
			} else {
				view->camera.state = robo::Camera::State::BIRDS_EYE_PAN_ROBOT;
			}
			break;
		}
		case hk::top_cam.value: {
			if(view->camera.state == robo::Camera::State::BIRDS_EYE) {
				view->camera.state = robo::Camera::State::BIRDS_EYE_PAN;
			} else {
				view->camera.state = robo::Camera::State::BIRDS_EYE;
			}
			break;
		}
		case hk::bot_cam.value: {
		case hk::bot_cam_fixed_orientation.value:
			robo::Camera::State match = event.keysym.sym == hk::bot_cam.value
				? robo::Camera::State::FOLLOW_ROBOT
				: robo::Camera::State::FOLLOW_ROBOT_FIXED_ORIENTATION
			;
			if(view->camera.state == match) {
				view->camera.next_robot(view->sim_state.robots);
			} else {
				view->camera.state = match;
			}
			break;
		}
		case hk::user_cam.value: {
			view->camera.state = robo::Camera::State::USER;
			break;
		}
		case hk::speed_up.value: {
			view->environment.add_speed_scale(1);
			break;
		}
		case hk::speed_down.value: {
			view->environment.add_speed_scale(-1);
			break;
		}
		case hk::toggle_all_labels.value: {
			view->robots_view.toggle_select_mode(view->robots_view.request_mode_labels);
			break;
		}
		case hk::toggle_all_velocities.value: {
			view->robots_view.toggle_select_mode(view->robots_view.request_mode_velocities);
			break;
		}
		case hk::toggle_all_reference_velocities.value: {
			view->robots_view.toggle_select_mode(view->robots_view.request_mode_reference_velocities);
			break;
		}
		case hk::toggle_all_rays.value: {
			view->robots_view.toggle_select_mode(view->robots_view.request_mode_rays);
			break;
		}
		case hk::toggle_all_histories.value: {
			view->robots_view.toggle_select_mode(view->robots_view.request_mode_histories);
			break;
		}
		case hk::toggle_all_debug_lines.value: {
			view->robots_view.toggle_select_mode(view->robots_view.request_mode_debug_lines);
			break;
		}
		case hk::toggle_all_coordinate_systems.value: {
			view->robots_view.toggle_select_mode(view->robots_view.request_mode_coordinate_systems);
			break;
		}
		case hk::reset_robot_cam_center.value: {
			view->camera.birds_eye_pan_robot_eye[0] = 0.0;
			view->camera.birds_eye_pan_robot_eye[1] = 0.0;
			view->camera.local_robot_camera.arm.position = Vertex<double,3>{0.0, 0.0, 0.0};
			view->camera.local_robot_camera.on_update();
			break;
		}
	}
	return true;
}

inline bool handle_window(SDL_WindowEvent event, robo::EnvironmentView* view) {
	switch(event.event) {
		case SDL_WINDOWEVENT_RESIZED:
		case SDL_WINDOWEVENT_SIZE_CHANGED: {
			view->camera.reshape(event.data1, event.data2);
			break;
		}
		case SDL_WINDOWEVENT_CLOSE: {
			return false;
		}
		case SDL_WINDOWEVENT_SHOWN:
		case SDL_WINDOWEVENT_EXPOSED:
		case SDL_WINDOWEVENT_RESTORED: {
			break;
		}
		case SDL_WINDOWEVENT_HIDDEN:
		case SDL_WINDOWEVENT_MINIMIZED: {
			break;
		}
		default: ;
	}
	return true;
}

inline void event_loop(robo::Environment& environment, double delta_t_gui, std::atomic<bool> & is_running, bool has_imgui) {
	name_this_thread("eloop");
	robo::EnvironmentView view(environment, 1024, 768, has_imgui);
	int const action_cam_frames_per_mode = 60*5;
	std::optional<std::variant<robo::RobotId, std::size_t>> active_object;
	int mouse_x;
	int mouse_y;
	bool is_mod_ctrl  = false;
	bool is_mod_shift = false;
	bool is_mod_alt   = false;
	std::optional<Vertex<int,2>> mouse_on_begin_mod_ctrl;
	std::optional<Vertex<int,2>> mouse_on_begin_mod_shift;
	view.camera.state = robo::Camera::State::USER;
	while( is_running ) {
		using clock_t = std::chrono::steady_clock;
		auto loop_enter = clock_t::now();
		auto find_view = [&](uint32_t window_id) {
			return view.window.window_id == window_id
				? &view
				: nullptr
			;
		};
		{
			SDL_Keymod mod = SDL_GetModState();
			is_mod_ctrl  = mod & KMOD_CTRL;
			is_mod_shift = mod & KMOD_SHIFT;
			is_mod_alt   = mod & KMOD_ALT;
		}

		SDL_Event e;
		while( SDL_PollEvent( &e ) != 0 ) {
			if(has_imgui) {
				ImGui_ImplSDL2_ProcessEvent(&e);
			}
			switch(e.type) {
				case SDL_QUIT: {
					is_running = false;
					break;
				}
				case SDL_KEYDOWN : {
					break;
				}
				case SDL_KEYUP: {
					if(!handle_key(e.key, find_view(e.key.windowID))) {
						is_running = false;
					}
					break;
				}
				case SDL_WINDOWEVENT: {
					if(!handle_window(e.window, find_view(e.key.windowID))) {
						is_running = false;
					}
					break;
				}
				case SDL_MOUSEBUTTONDOWN: {
					Vertex<double,3> mouse_drag = find_view(e.key.windowID)->camera.world_position(mouse_x, mouse_y);
					if(e.button.button == SDL_BUTTON_LEFT) {
						active_object = {};
						if(environment.obstacles.movable_obstacles) {
							if(auto o_o = environment.closest_obstacle(mouse_drag)
								; o_o
							) {
								active_object = *o_o;
							}
						}
						if(auto o_r = environment.closest_robot(mouse_drag); o_r) {
							auto [distance, robot_id] = *o_r;
							double eps = 2.0*robo::config::Robot::radius;
							if(distance <= eps) {
								active_object = robot_id;
							}
						}
					} else if(e.button.button == SDL_BUTTON_RIGHT) {
						if(auto o_r = environment.closest_robot(mouse_drag); o_r) {
							auto [distance, robot_id] = *o_r;
							double eps = 2.0*robo::config::Robot::radius;
							if(distance <= eps) {
								environment.toggle_robot_pause(robot_id);
							}
						}
					}
					break;
				}
				case SDL_MOUSEBUTTONUP: {
					active_object = {};
					break;
				}
				case SDL_MOUSEWHEEL: {
					double k = 1.05;
					if(e.wheel.y > 0) {
						k = 1.0 / k;
					}
					view.camera.scale_arm_length(k);
					break;
				}
				case SDL_MOUSEMOTION: {
					Vertex<double,3> mouse_at = find_view(e.key.windowID)->camera.world_position(e.motion.x, e.motion.y);
					if(view.gui) {
						view.gui->mouse_at = mouse_at;
					}
					if(    active_object
						|| (   !is_mod_alt
							&& !is_mod_ctrl
							&& !is_mod_shift
						)
					) {
						mouse_x = e.motion.x;
						mouse_y = e.motion.y;
					}
					if(active_object) {
						Vertex<double,2> p2{mouse_at[0], mouse_at[1]};
						if(std::holds_alternative<robo::RobotId>(*active_object)) {
							environment.move_robot(std::get<robo::RobotId>(*active_object),p2);
						}
						if(std::holds_alternative<std::size_t>(*active_object)) {
							environment.move_obstacle(std::get<std::size_t>(*active_object), p2);
						}
					} else if(is_mod_shift) {
						view.camera.move_left(-e.motion.xrel * 0.005);
// 						view.camera.move_down( -e.motion.yrel * 0.01);
						view.camera.move_forward(-e.motion.yrel * 0.005);
					} else if(is_mod_ctrl) {
						double d_azimuth = -e.motion.xrel * 0.001;
						double d_height  = -e.motion.yrel * 0.001;
						view.camera.rotate_arm(d_azimuth, d_height);
					}
// 					if(is_mod_shift) {
// 						view.camera.move_right(-e.motion.xrel * 0.01);
// 						view.camera.move_down( -e.motion.yrel * 0.01);
// 					} else if(is_mod_ctrl) {
// 						view.camera.pan_right( -e.motion.xrel * 0.001);
// 						view.camera.tilt_down( -e.motion.yrel * 0.001);
// 					} else if(is_mod_alt) {
// 						view.camera.roll_right(-e.motion.xrel * 0.001);
// 						view.camera.tilt_down( -e.motion.yrel * 0.001);
// 					} else if(active_object) {
// 						Vertex<double,3> mouse_drag = find_view(e.key.windowID)->camera.world_position(mouse_x, mouse_y);
// 						Vertex<double,2> p2{mouse_drag[0], mouse_drag[1]};
// 						environment.moveRobot(*active_object,p2);
// 					}
					break;
				}
			}
		}
		view.render(is_mod_alt || is_mod_ctrl || is_mod_shift);
		auto deadline  = loop_enter + std::chrono::duration<double>(delta_t_gui);
		std::this_thread::sleep_until(deadline);
	}
}

