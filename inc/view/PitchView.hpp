#pragma once
#include "config/Pitch.hpp"
#include "config/Textures.hpp"
#include "simple_gl/make_simple_cone.hpp"
#include "simple_gl/make_checker_board.hpp"
#include "simple_gl/Texture.hpp"
#include "math/PointLine.hpp"
#include "config/View.hpp"
#include <vector>
#include "view/EnvironmentViewBase.hpp"

namespace robo {

struct PitchView {
	using mesh_data_t_plane_cube = mesh_data_by_tags_t<has_normal_tag>;
	using mesh_data_t_plane      = mesh_data_by_tags_t<has_normal_tag, has_tex_tag>;
	using gl_mesh_t_plane_cube   = mesh_data_t_plane_cube::gl_mesh_t;
	using gl_mesh_t_plane        = mesh_data_t_plane::gl_mesh_t;

	EnvironmentViewBase&         parent;
	Texture const                texture_plane_A;
	Texture const                texture_plane_B;
	mesh_data_t_plane_cube const mesh_data_plane_cube;
	mesh_data_t_plane      const mesh_data_plane_A;
	mesh_data_t_plane      const mesh_data_plane_B;
	gl_mesh_t_plane_cube   const mesh_plane_cube{mesh_data_plane_cube.mesh(&parent.window)};
	gl_mesh_t_plane        const mesh_plane_A{   mesh_data_plane_A.mesh(     &parent.window)};
	gl_mesh_t_plane        const mesh_plane_B{   mesh_data_plane_B.mesh(     &parent.window)};
	
	PitchView(EnvironmentViewBase& parent)
		: parent{parent}
		, texture_plane_A{make_checker_board(&parent.window, 20, 20, 1200, 800,  64)}
		, texture_plane_B{&parent.window, config::Textures::X_2k_mercury}
		, mesh_data_plane_cube{make_simple_cube<false,false>(config::Pitch::width, config::Pitch::height, 0.5f)}
		, mesh_data_plane_A{make_square_xy<true,false>(config::Pitch::width, config::Pitch::height, -0.001)}
		, mesh_data_plane_B{make_square_xy<true,false>(config::Pitch::width, config::Pitch::height, -0.015)}
	{}
	
	void show() {
		Vertex<float, 4> color = parent.sim_state.is_paused
			? Vertex<float, 4>{0.4f,0.4f,0.4f,1.0f}
			: Vertex<float, 4>{0.2f,0.2f,0.3f,1.0f}
		;
		scoped_draw([&]() {
			auto lock = mesh_plane_cube.lock();
			glTranslated(0.0f, 0.0f, -0.28);
			parent.set_color(color);
			lock.draw();
		});
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		{
			texture_plane_B.bind();
			parent.set_color(Vertex<double,4>{1.0,1.0,1.0,0.3});
			auto lock = mesh_plane_B.lock();
			lock.draw();
		}
		{
			texture_plane_A.bind();
			parent.set_color(Vertex<double,4>{1.0,1.0,1.0,0.3});
			auto lock = mesh_plane_A.lock();
			lock.draw();
		}
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_TEXTURE_2D);
	}
};

} /** namespace robo */
