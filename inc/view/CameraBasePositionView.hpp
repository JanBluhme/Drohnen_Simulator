#pragma once
#include "simple_gl/make_simple_cone.hpp"
#include "config/View.hpp"
#include "view/EnvironmentViewBase.hpp"
#include "view/Camera.hpp"
#include "view/RobotsView.hpp"

namespace robo {

struct CameraBasePositionView {
	using mesh_data_t = mesh_data_by_tags_t<has_normal_tag>;
	using gl_mesh_t   = mesh_data_t::gl_mesh_t;

	EnvironmentViewBase&   parent;
	Camera const&          camera;
	RobotsView const&      robots_view;
	double                 phi = 0.0;

	mesh_data_t const mesh_data;
	gl_mesh_t const   mesh{mesh_data.mesh(&parent.window)};

	CameraBasePositionView(EnvironmentViewBase& parent, Camera const& camera, RobotsView const& robots_view)
		: parent{parent}
		, camera{camera}
		, robots_view{robots_view}
		, mesh_data{[](){
			return
				  Transform::translate(0.0, 0.0, 0.01)
				* (  make_simple_ring<false,false>(0.2, 0.3, 0.0, 1.5 * M_PI, 24)
				   + make_simple_disk<false, false>(0.1, 0.0, 2.0 * M_PI, 24)
				)
			;
		}()}
	{}

	void show(bool is_show) {
		phi += 10.0;
		if(is_show) {
			scoped_draw([&]() {
				Vertex<float, 4> color{0.4f, 0.4f, 0.7f, 0.8f};
				if(    camera.state == Camera::State::FOLLOW_ROBOT_ROTATING
					|| camera.state == Camera::State::FOLLOW_ROBOT
					|| camera.state == Camera::State::FOLLOW_ROBOT_FIXED_ORIENTATION
					|| camera.state == Camera::State::BIRDS_EYE_PAN_ROBOT
				) {
					Robot* r = parent.sim_state.robots.find(camera.tracked_robot);
					if(r) {
						color = robots_view.robot_color(*r, 1.0, 0.5);
					}
				}
				auto p = camera.camera_base_posion();
				glTranslated(p[0], p[1], p[2]);
				glRotated(phi, 0.0, 0.0, 1.0);
				//parent.set_color(Vertex<double,4>{1.0,1.0,1.0,1.0});
				parent.set_color(color);
				auto lock = mesh.lock();
				lock.draw();
			});
		}
	}
};

} /** namespace robo */
