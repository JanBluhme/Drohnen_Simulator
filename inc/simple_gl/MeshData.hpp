#pragma once
#include "simple_gl/BoundingBox.hpp"
#include "simple_gl/Mesh.hpp"
#include "math/r3/Triangle.hpp"
#include "math/r3/Transform.hpp"
#include <vector>

template<bool has_normal, bool has_tex, bool has_color>
struct MeshData {
	using vertex_t  = MeshVertex<double, has_normal, has_tex, has_color>;
	using gl_mesh_t = Mesh<typename vertex_t::template as_type<float>>;

	std::vector<vertex_t>   vertices;
	std::vector<unsigned int> indices;

	/*MeshData() = default;
	MeshData(std::vector<vertex_t> const& vertices, std::vector<unsigned int> const& indices)
		: vertices{vertices}
		, indices{indices}
	{
		for(auto i : indices) {
			assert(i < vertices.size());
		}
	}*/

	auto mesh(GL_Window* parent) const
		-> gl_mesh_t
	{
		using target_t = typename vertex_t::template as_type<float>;
		using Mesh = Mesh<target_t>;

		auto convert_vertices = [&]() {
			std::vector<target_t> target_vertices;
			target_vertices.reserve(vertices.size());
			for(auto const& v : vertices) {
				target_vertices.push_back(v.template as<float>());
			}
			return target_vertices;
		};
		auto may_convert_vertices = [&]()
			-> decltype(auto)
		{
			if constexpr(std::is_same_v<vertex_t, target_t>) {
				return static_cast<std::vector<vertex_t> const&>(vertices);
			} else {
				return convert_vertices();
			}
		};
		auto convert_indices = [&]() {
			std::vector<unsigned short> result;
			result.reserve(indices.size());
			std::copy(indices.begin(), indices.end(), std::back_inserter(result));
			return result;
		};
		if(vertices.size() < std::numeric_limits<unsigned short>::max()) {
			return Mesh(parent, may_convert_vertices(), convert_indices());
		} else {
			return Mesh(parent, may_convert_vertices(), indices);
		}
	}

	auto triangles() const
		-> std::vector<Triangle>
	{
		std::vector<Triangle> result;
		result.reserve(indices.size()/3);
		for(std::size_t i = 0; i < indices.size(); i += 3) {
			auto get_vertex = [&](std::size_t index_index)
				-> vertex_t const&
			{
				return vertices[indices[index_index]];
			};
			result.emplace_back(
				  get_vertex(i + 0).position
				, get_vertex(i + 1).position
				, get_vertex(i + 2).position
			);
		}
		return result;
	}

	friend
	auto operator*(Transform const& T, MeshData const& mesh)
		-> MeshData
	{
		MeshData result;
		result.indices = mesh.indices;
		result.vertices.reserve(mesh.vertices.size());
		for(auto const& v : mesh.vertices) {
			vertex_t vv;
			if constexpr(vertex_t::has_position) {
				vv.position = T.position(v.position);
			}
			if constexpr(vertex_t::has_normal) {
				vv.normal = T.direction(v.normal);
			}
			if constexpr(vertex_t::has_tex) {
				vv.tex = v.tex;
			}
			if constexpr(vertex_t::has_color) {
				vv.color = v.color;
			}
			result.vertices.push_back(vv);
		}
		return result;
	}
	friend
	auto operator+=(MeshData& A, MeshData const& B)
		-> MeshData&
	{
		std::size_t const ofs = A.vertices.size();
		A.vertices.reserve(A.vertices.size() + B.vertices.size());
		std::copy(
			  B.vertices.begin()
			, B.vertices.end()
			, std::back_inserter(A.vertices)
		);
		A.indices.reserve(A.indices.size() + B.indices.size());
		std::transform(
			  B.indices.begin()
			, B.indices.end()
			, std::back_inserter(A.indices)
			, [&](auto i) {
				return i + ofs;
			}
		);
		return A;
	}
	friend
	auto operator+(MeshData const& A, MeshData const& B)
		-> MeshData
	{
		MeshData AA{A};
		return AA += B;
	}
	constexpr auto bounding_box() const noexcept
		-> BoundingBox
		requires vertex_t::has_position
	{
		BoundingBox r;
		for(auto i : indices) {
			r.grow(vertices[i].position);
		}
		return r;
	}
};

struct has_normal_tag{};
struct has_tex_tag   {};
struct has_color_tag {};

template<typename... Ts>
struct TList {};

template<typename List, typename T>
inline constexpr bool contains = false;

template<typename... Ts, typename T>
inline constexpr bool contains<TList<Ts...>, T> = (std::is_same_v<T, Ts> || ...);

template<typename... tags>
	requires (
		(contains<TList<has_normal_tag, has_tex_tag, has_color_tag>, tags> && ...)
	)
using mesh_data_by_tags_t = MeshData<
	  contains<TList<tags...>, has_normal_tag>
	, contains<TList<tags...>, has_tex_tag>
	, contains<TList<tags...>, has_color_tag>
>;
