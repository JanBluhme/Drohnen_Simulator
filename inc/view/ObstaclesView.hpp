#pragma once
#include "environment_models/ObstacleSet.hpp"
#include "math/PointLine.hpp"
#include "config/View.hpp"
#include "simple_gl/make_checker_board.hpp"
#include <vector>
#include <map>
#include <iostream>
#include "view/EnvironmentViewBase.hpp"

namespace robo {

struct ObstaclesView {
	using source_t = ObstacleSet::exchange_t;

	using mesh_data_t = typename ObstacleGeometry::mesh_data_t;

	struct ObstacleSetMeshes {
		typename mesh_data_t::gl_mesh_t mesh_original;
		typename mesh_data_t::gl_mesh_t mesh_collision;
		typename mesh_data_t::gl_mesh_t mesh_collision_bounding_box;

		ObstacleSetMeshes(
			  ObstaclesView&    parent
			, ObstacleSet::ObstacleClass const& geometry
		)
			: mesh_original{              geometry.raw.mesh_data.mesh(               &parent.parent.window)}
			, mesh_collision{             geometry.grown.mesh_data.mesh(             &parent.parent.window)}
			, mesh_collision_bounding_box{geometry.grown.bounding_box_mesh_data.mesh(&parent.parent.window)}
		{}
	};

	using meshes_map_t = std::map<ObstacleSet::class_id_t, ObstacleSetMeshes>;

	EnvironmentViewBase& parent;
	meshes_map_t         meshes;
	Texture              texture = make_checker_board(
		  &parent.window
		, 7, 9
		, 32, 32
		, Vertex<double, 4>{0.0, 0.0, 1.0, 1.0}
		, Vertex<double, 4>{0.7, 0.4, 0.0, 0.4}
	);
	bool                 show_original      = true;
	bool                 show_texture       = true;
	bool                 show_collision     = false;
	bool                 show_bounding_box  = false;
	double               alpha_original     = 1.0;
	double               alpha_collision    = 0.25;
	double               alpha_bounding_box = 0.125;


	ObstaclesView(EnvironmentViewBase& parent)
		: parent{parent}
	{}

	void show() {
		source_t const& obstacles = parent.sim_state.obstacles;
		for(auto const& p : obstacles) {
			auto it = meshes.find(p.first);
			if(it == meshes.end()) {
				it = meshes.emplace(
					  std::piecewise_construct
					, std::forward_as_tuple(p.first)
					, std::forward_as_tuple(*this, *p.first)
				).first;
			}
			ObstacleSetMeshes const& osm = it->second;
			Transform const& pose = p.second;
			scoped_draw([&]() {
				glMultMatrixd(pose.as_gl_matrix().data());
				if(show_original) {
					if(show_texture) {
						glEnable(GL_TEXTURE_2D);
						texture.bind();
						auto lock = osm.mesh_original.lock();
						parent.set_color(Vertex<double,4>{1.0,1.0,1.0,alpha_original});
						lock.draw();
						glDisable(GL_TEXTURE_2D);
					} else {
						auto lock = osm.mesh_original.lock();
						parent.set_color(Vertex<double,4>{0.9,0.2,0.3,alpha_original});
						lock.draw();
					}
				}
				if(show_collision) {
					auto lock = osm.mesh_collision.lock();
					parent.set_color(Vertex<double,4>{1.0,1.0,1.0,alpha_collision});
					lock.draw();
				}
				if(show_bounding_box) {
					auto lock = osm.mesh_collision_bounding_box.lock();
					double scale = 1.005;
					glScaled(scale, scale, scale);
					parent.set_color(Vertex<double,4>{1.0,0.0,0.0,alpha_bounding_box});
					lock.draw();
				}
			});
		}
	}
};

} /** namespace robo */
