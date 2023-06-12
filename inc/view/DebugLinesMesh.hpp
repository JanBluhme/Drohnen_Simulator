#pragma once
#include "robo_commands.hpp"
#include "util/iterator_of.hpp"
#include "simple_gl/MeshData.hpp"

namespace robo {

struct DebugLinesMesh {
	std::vector<unsigned int>                       index_buffer;
	std::vector<MeshVertex<float, true,false,true>> vertex_buffer;
	Mesh<MeshVertex<float, true, false, true>>      mesh;

	DebugLinesMesh(GL_Window* parent)
		: mesh{parent}
	{}
	template<iterator_of<DebugLine> iterator>
	void show(iterator first, iterator last) {
		std::size_t N = std::distance(first, last);
		if(!N) {
			return;
		}
		index_buffer.clear();
		vertex_buffer.clear();
		index_buffer.resize( 6 * N);
		vertex_buffer.resize(4 * N);
		unsigned int line = 0;
		auto add_line = [&](DebugLine const& l) {
			index_buffer[6 * line + 0] =  4 * line + 0;
			index_buffer[6 * line + 1] =  4 * line + 1;
			index_buffer[6 * line + 2] =  4 * line + 2;
			index_buffer[6 * line + 3] =  4 * line + 0;
			index_buffer[6 * line + 4] =  4 * line + 2;
			index_buffer[6 * line + 5] =  4 * line + 3;
			auto& A = vertex_buffer[4 * line + 0];
			auto& B = vertex_buffer[4 * line + 1];
			auto& C = vertex_buffer[4 * line + 2];
			auto& D = vertex_buffer[4 * line + 3];

			auto make_float_vertex = [](std::floating_point auto... vs) {
				return Vertex<float, sizeof...(vs)>{static_cast<float>(vs)...};
			};

			Vertex<float,4> color_a = make_float_vertex(l.color_start[0], l.color_start[1], l.color_start[2], 1.0f);
			Vertex<float,4> color_b = make_float_vertex(l.color_end[  0], l.color_end[  1], l.color_end[  2], 1.0f);
			Vertex<double,2> n = ortho(
				Vertex<double,2>{
					  l.end[0] - l.start[0]
					, l.end[1] - l.start[1]
				}
			);
			double n_len = n.length();
			if(n_len > 0.0) {
				n *= l.thickness / (2.0 * n_len);
			} else {
				n = Vertex<double,2>{1.0f, 0.0f} * (l.thickness / 2.0);
			}
			A.color = color_a;
			B.color = color_a;
			C.color = color_b;
			D.color = color_b;
			A.normal = {0.0f,0.0f,1.0f};
			B.normal = A.normal;
			C.normal = A.normal;
			D.normal = A.normal;
			A.position = make_float_vertex(l.start[0] - n[0], l.start[1] - n[1], l.start[2]);
			B.position = make_float_vertex(l.start[0] + n[0], l.start[1] + n[1], l.start[2]);
			C.position = make_float_vertex(l.end[0]   + n[0], l.end[1]   + n[1], l.end[2]  );
			D.position = make_float_vertex(l.end[0]   - n[0], l.end[1]   - n[1], l.end[2]  );
			++line;
		};
		for(; first != last; ++first) {
			add_line(*first);
		}
		mesh.load(vertex_buffer, index_buffer, GL_DYNAMIC_DRAW);
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
