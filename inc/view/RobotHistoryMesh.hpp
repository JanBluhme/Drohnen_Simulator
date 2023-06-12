#pragma once
#include <array>
#include <vector>
#include "math/Vertex.hpp"
#include "simple_gl/Mesh.hpp"
#include "simple_gl/GL_Window.hpp"
#include "Robot.hpp"
#include "config/Robot.hpp"

namespace robo {
struct RobotHistoryMesh {
	using mesh_vertex_t = MeshVertex<float, true, false, true>;
	using mesh_t        = Mesh<mesh_vertex_t>;
	std::size_t                 size = 2048;
	std::size_t                 history_idx = 0;
	mesh_t                      mesh;
	std::vector<mesh_vertex_t>  vertices;
	std::vector<unsigned short> indices;
	bool                        do_reset = true;
	static constexpr double     path_max_height = 0.05;
	double                      alpha = 1.0;

	static std::size_t num_vertices(std::size_t history_size) {
		return 2 * history_size;
	}
	static std::size_t num_indices(std::size_t history_size) {
		return history_size
			? 6 * (history_size - 1)
			: 0
		;
	}

	RobotHistoryMesh(GL_Window* parent)
		: mesh{parent}
		, vertices(num_vertices(size))
		, indices(num_indices(size))
	{}

	void reset() {
		do_reset = true;
	}

	void resize(std::size_t new_size) {
		if(size != new_size) {
			if(size) {
				std::size_t last = (history_idx - 1) % size;
				mesh_vertex_t A = vertices[2 * last + 0];
				mesh_vertex_t B = vertices[2 * last + 1];
				indices.resize( num_indices(new_size));
				vertices.resize(num_vertices(new_size));
				size = new_size;
				for(std::size_t i = 2; i < vertices.size(); i += 2) {
					vertices[i + 0] = A;
					vertices[i + 1] = B;
				}
				history_idx = 0;
			} else {
				indices.resize( num_indices(new_size));
				vertices.resize(num_vertices(new_size));
				size = new_size;
				history_idx = 0;
			}
		}
	}

	void update(Robot const& robot, Vertex<float,4> const& color) {
		history_idx %= size;
		auto const& kinematics = robot.kinematics;
		auto const& p = kinematics.position;

		double phi = kinematics.velocity * kinematics.velocity > 1e-6
			? orientation(kinematics.velocity)
			: kinematics.orientation
		;
		auto n = polar(config::Robot::radius, phi + M_PI/2);

		auto Vf = [](Vertex<double, 2> const& vd) {
			return Vertex<float,2> {
				  static_cast<float>(vd[0])
				, static_cast<float>(vd[1])
			};
		};
		std::array<Vertex<float,2>,2> entry{Vf(p - n), Vf(p + n)};

		auto dofoo = [&]() {
			double h = 0.01;
			mesh_vertex_t& vl = vertices[2*history_idx + 0];
			mesh_vertex_t& vr = vertices[2*history_idx + 1];

			vl.position[0] = entry[0][0];
			vl.position[1] = entry[0][1];
			vr.position[0] = entry[1][0];
			vr.position[1] = entry[1][1];

			vl.normal = vr.normal = Vertex<float,3>{0.0f, 0.0f, 1.0f};
			vl.color  = vr.color  = color;//Vertex<float,4>{1.0f, 1.0f, 1.0f, 1.0f};
			alpha = color[3];
		};
		if(do_reset) {
			for(history_idx = 0; history_idx < size; ++history_idx) {
				dofoo();
			}
			history_idx = 0;
			do_reset = false;
		} else {
			dofoo();
		}
		++history_idx;
		history_idx %= size;
	}

	void update_indices() {
		history_idx %= size;
		for(std::size_t i = 1; i < size; ++i) {
			std::size_t ii = i - 1;
			std::size_t h_idx0 = (i + history_idx + 0) % size;
			std::size_t h_idx1 = (i + history_idx - 1) % size;

			indices[6 * ii +  0] = 2*h_idx0;
			indices[6 * ii +  1] = 2*h_idx0 + 1;
			indices[6 * ii +  2] = 2*h_idx1 + 1;

			indices[6 * ii +  3] = 2*h_idx0;
			indices[6 * ii +  4] = 2*h_idx1 + 1;
			indices[6 * ii +  5] = 2*h_idx1;
		}
	}

	void update_slope() {
		history_idx %= size;
		for(std::size_t i = 1; i < size; ++i) {
			std::size_t h_idx0 = (i + history_idx + 0) % size;
			std::size_t h_idx1 = (i + history_idx - 1) % size;
			double kh = path_max_height;
			double h0 = i * kh / size;
			double h1 = (i+1) * kh / size;
			vertices[2*h_idx1 + 0].position[2] = h1;
			vertices[2*h_idx1 + 1].position[2] = h1;
			vertices[2*h_idx0 + 0].position[2] = h0;
			vertices[2*h_idx0 + 1].position[2] = h0;
		}
	}
	void update_alpha() {
		history_idx %= size;
		for(std::size_t i = 1; i < size; ++i) {
			std::size_t h_idx0 = (i + history_idx + 0) % size;
			std::size_t h_idx1 = (i + history_idx - 1) % size;
			double k_alpha = alpha;
			double alpha_0 = (i - 1) * k_alpha / size;
			double alpha_1 =  i      * k_alpha / size;
			vertices[2*h_idx1 + 0].color[3] = alpha_1;
			vertices[2*h_idx1 + 1].color[3] = alpha_1;
			vertices[2*h_idx0 + 0].color[3] = alpha_0;
			vertices[2*h_idx0 + 1].color[3] = alpha_0;
		}
	}

	void show() {
		update_indices();
		update_slope();
		update_alpha();
		mesh.load(vertices, indices, GL_DYNAMIC_DRAW);
		scoped_draw([&]() {
			glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
			glEnable(GL_COLOR_MATERIAL);
			auto lock = mesh.lock();
			lock.draw();
			glDisable(GL_COLOR_MATERIAL);
		});
	}
};
} /* namespace robo */
