#pragma once
#include "view/EnvironmentViewBase.hpp"
#include "simple_gl/make_simple_cone.hpp"

namespace robo {

struct CoordinateSystemComponent {
	using mesh_data_t = mesh_data_by_tags_t<has_normal_tag>;
	using gl_mesh_t   = mesh_data_t::gl_mesh_t;
	EnvironmentViewBase& parent;
	mesh_data_t const    mesh_data;
	gl_mesh_t   const    mesh{mesh_data.mesh(&parent.window)};

	CoordinateSystemComponent(EnvironmentViewBase& parent, double length, double radius)
		: parent(parent)
		, mesh_data(make_simple_cone<false,false>(radius, radius / 5.0, 0.0, length, 12, false, false))
	{}

	void show() const {
		auto lock = mesh.lock();
		scoped_draw([&]() {
			glTranslated(0.0, 0.0, 0.0);
			parent.set_color(Vertex<double,4>{0.5, 0.0, 0.0, 0.5});
			glRotated(90.0, 0.0, 1.0, 0.0);
			lock.draw();
		});
		scoped_draw([&]() {
			glTranslated(0.0, 0.0, 0.0);
			parent.set_color(Vertex<double,4>{0.0, 0.5, 0.0, 0.5});
			glRotated(-90.0, 1.0, 0.0, 0.0);
			lock.draw();
		});
		scoped_draw([&]() {
			glTranslated(0.0, 0.0, 0.0);
				parent.set_color(Vertex<double,4>{0.0, 0.0, 0.5, 0.25});
			lock.draw();
		});
	}
};

} /* namespace robo */
