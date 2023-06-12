#pragma once
#include <GL/glew.h>
#include <memory>
#include <vector>
#include <cassert>
#include "math/Vertex.hpp"
#include "simple_gl/GL_Window.hpp"
#include <type_traits>

template<typename T>
	requires std::is_same_v<T, float>
		  || std::is_same_v<T, double>
constexpr auto get_gl_type() noexcept {
	if constexpr(std::is_same_v<T, float>) {
		return GL_FLOAT;
	} else {
		return GL_DOUBLE;
	}
}

template<typename T, bool>
struct MeshVertexPosition {
	static constexpr bool has_position = false;

	template<typename TT>
	constexpr auto position_as() const noexcept
		-> MeshVertexPosition<TT, false>
	{
		return {};
	}
};
template<typename T>
struct MeshVertexPosition<T, true> {
	using underlying_type_position         = T;
	using vertex_t_position                = Vertex<T, 3>;
	static constexpr bool has_position     = true;
	static constexpr auto gl_type_position = get_gl_type<T>();
	vertex_t_position     position;

	template<typename TT>
	constexpr auto position_as() const noexcept
		-> MeshVertexPosition<TT, true>
	{
		return {position.template as_vertex_of<TT>()};
	}
};

template<typename T, bool>
struct MeshVertexNormal {
	static constexpr bool has_normal = false;

	template<typename TT>
	constexpr auto normal_as() const noexcept
		-> MeshVertexNormal<TT, false>
	{
		return {};
	}
};
template<typename T>
struct MeshVertexNormal<T, true> {
	using underlying_type_normal         = T;
	using vertex_t_normal                = Vertex<T, 3>;
	static constexpr bool has_normal     = true;
	static constexpr auto gl_type_normal = get_gl_type<T>();
	vertex_t_normal       normal;

	template<typename TT>
	constexpr auto normal_as() const noexcept
		-> MeshVertexNormal<TT, true>
	{
		return {normal.template as_vertex_of<TT>()};
	}
};

template<typename T, bool>
struct MeshVertexTex {
	static constexpr bool has_tex = false;

	template<typename TT>
	constexpr auto tex_as() const noexcept
		-> MeshVertexTex<TT, false>
	{
		return {};
	}
};
template<typename T>
struct MeshVertexTex<T, true> {
	using underlying_type_tex         = T;
	using vertex_t_tex                = Vertex<T, 2>;
	static constexpr bool has_tex     = true;
	static constexpr auto gl_type_tex = get_gl_type<T>();
	vertex_t_tex          tex;

	template<typename TT>
	constexpr auto tex_as() const noexcept
		-> MeshVertexTex<TT, true>
	{
		return {tex.template as_vertex_of<TT>()};
	}
};

template<typename T, bool>
struct MeshVertexColor {
	static constexpr bool has_color = false;

	template<typename TT>
	constexpr auto color_as() const noexcept
		-> MeshVertexColor<TT, false>
	{
		return {};
	}
};
template<typename T>
struct MeshVertexColor<T, true> {
	using underlying_type_color         = T;
	using vertex_t_color                = Vertex<T, 4>;
	static constexpr bool has_color     = true;
	static constexpr auto gl_type_color = get_gl_type<T>();
	vertex_t_color        color;

	template<typename TT>
	constexpr auto color_as() const noexcept
		-> MeshVertexColor<TT, true>
	{
		return {color.template as_vertex_of<TT>()};
	}
};

template<typename T, bool has_normal, bool has_tex, bool has_color>
struct MeshVertex
	: MeshVertexPosition<T, true>
	, MeshVertexNormal<  T, has_normal>
	, MeshVertexTex<     T, has_tex>
	, MeshVertexColor<   T, has_color>
{
	template<typename TT>
	using as_type = MeshVertex<TT, has_normal, has_tex, has_color>;

	MeshVertex() = default;
	MeshVertex(
		  MeshVertexPosition<T, true      >const& position
		, MeshVertexNormal<  T, has_normal>const& normal
		, MeshVertexTex<     T, has_tex   >const& tex
		, MeshVertexColor<   T, has_color >const& color
	)
		: MeshVertexPosition<T, true      >{position}
		, MeshVertexNormal<  T, has_normal>{normal  }
		, MeshVertexTex<     T, has_tex   >{tex     }
		, MeshVertexColor<   T, has_color >{color   }
	{}

	template<typename TT>
	constexpr as_type<TT> as() const noexcept {
		if constexpr(std::is_same_v<TT, T>) {
			return *this;
		} else {
			return {
				  this->template position_as<TT>()
				, this->template normal_as<  TT>()
				, this->template tex_as<     TT>()
				, this->template color_as<   TT>()
			};
		}
	}
};

namespace sad {
	template<typename...>
	struct Fail_base;

	template<typename... Ts>
	struct Fail
		: Fail_base<Ts...>
	{};
}

template<typename Vertex>
struct Mesh {
	GL_Window const*  parent;
	unsigned int      vertex_VBO_id;
	unsigned int      index_VBO_id;
	std::size_t       indices_size;
	GLenum            type;
	
	Mesh(Mesh const&) = delete;
	Mesh& operator=(Mesh const&) = delete;
	
	Mesh(Mesh&& other)
		: parent(other.parent)
		, vertex_VBO_id(other.vertex_VBO_id)
		, index_VBO_id(other.index_VBO_id)
		, indices_size(other.indices_size)
	{
		other.vertex_VBO_id = 0;
		other.index_VBO_id  = 0;
	}
	
	bool has_buffers() const {
		return vertex_VBO_id != 0
			&& index_VBO_id  != 0
		;
	}

	Mesh& operator=(Mesh&& other) {
		if(&other != this) {
			assert(other.parent == parent && "Mesh moved to wrong GL_Context");
			vertex_VBO_id = other.vertex_VBO_id;
			index_VBO_id  = other.index_VBO_id;
			indices_size  = other.indices_size;
			other.vertex_VBO_id = 0;
			other.index_VBO_id  = 0;
		}
		return *this;
	}
	
	~Mesh() {
		parent->do_operation(
			[&]() {
				if(vertex_VBO_id) {
					glDeleteBuffers(1, &vertex_VBO_id);
				}
				if(index_VBO_id) {
					glDeleteBuffers(1, &index_VBO_id);
				}
			}
		);
	}
	
	Mesh(GL_Window const* parent)
		: parent(parent)
		, vertex_VBO_id(0)
		, index_VBO_id(0)
		, indices_size(0)
	{
		parent->do_operation(
			[&]() {
				glGenBuffers(1, &vertex_VBO_id);
				glGenBuffers(1, &index_VBO_id);
			}
		);
	}
	
	template<typename vertex_id_t>
	Mesh( GL_Window const* parent
		, std::vector<Vertex> const&      vertices
		, std::vector<vertex_id_t> const& indices
		, GLenum                          usage = GL_STATIC_DRAW
	)
		: Mesh(parent)
	{
		load<vertex_id_t>(vertices, indices, usage);
	}
	
	template<typename vertex_id_t>
	void load(
		  std::vector<Vertex> const&      vertices
		, std::vector<vertex_id_t> const& indices
		, GLenum                          usage = GL_STATIC_DRAW
	) {
		if(!has_buffers()) {
			return;
		}
		if constexpr(std::is_same_v<vertex_id_t, unsigned short>) {
			type = GL_UNSIGNED_SHORT;
		} else if constexpr(std::is_same_v<vertex_id_t, unsigned int>) {
			type = GL_UNSIGNED_INT;
		} else {
			sad::Fail<vertex_id_t> x;
		}

		parent->do_operation(
			[&]() {
				glBindBuffer(GL_ARRAY_BUFFER, vertex_VBO_id);
				glBufferData(
					  GL_ARRAY_BUFFER
					, sizeof(Vertex) * vertices.size()
					, vertices.data()
					, usage
				);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_VBO_id);
				glBufferData(
					  GL_ELEMENT_ARRAY_BUFFER
					, sizeof(vertex_id_t) * indices.size()
					, indices.data()
					, usage
				);

				indices_size = indices.size();
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
		);
	}

	struct Lock {
		Mesh const* parent;
		
		Lock(Lock const&) = delete;
		Lock& operator=(Lock const&) = delete;
		
		Lock(Lock&& other)
			: parent(other.parent)
		{
			other.parent = nullptr;
		}
		Lock& operator=(Lock&& other) {
			if(&other != this) {
				parent = other.parent;
				other.parent = nullptr;
			}
			return *this;
		}
		Lock(Mesh const* parent)
			: parent(parent)
		{
			static constexpr Vertex probe{};
			auto buffer_offset = [](Vertex const& probe, auto const& probe_member) {
				std::size_t diff
					= reinterpret_cast<char const*>(&probe_member)
					- reinterpret_cast<char const*>(&probe)
				;
				return reinterpret_cast<void*>(diff);
			};
			glBindBuffer(GL_ARRAY_BUFFER, parent->vertex_VBO_id);

			constexpr static auto TYPE = GL_FLOAT;
			
			if constexpr(Vertex::has_position) {
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(  3, Vertex::gl_type_position, sizeof(Vertex), buffer_offset(probe, probe.position));
			}
			
			if constexpr(Vertex::has_tex) {
				glClientActiveTexture(GL_TEXTURE0);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, Vertex::gl_type_tex, sizeof(Vertex), buffer_offset(probe, probe.tex));
			}
			
			if constexpr(Vertex::has_normal) {
				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer(     Vertex::gl_type_normal, sizeof(Vertex), buffer_offset(probe, probe.normal));
			}
			
			if constexpr(Vertex::has_color) {
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(   4, Vertex::gl_type_color, sizeof(Vertex), buffer_offset(probe, probe.color));
			}
			
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, parent->index_VBO_id);
		}
		~Lock() {
			if constexpr(Vertex::has_position) {
				glDisableClientState(GL_VERTEX_ARRAY);
			}
			if constexpr(Vertex::has_tex) {
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}
			if constexpr(Vertex::has_normal) {
				glDisableClientState(GL_NORMAL_ARRAY);
			}
			if constexpr(Vertex::has_color) {
				glDisableClientState(GL_COLOR_ARRAY);
			}
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		void draw() const {
			if(parent && parent->has_buffers()) {
				glDrawElements(GL_TRIANGLES, parent->indices_size, parent->type, nullptr);
			}
		}
	};
	Lock lock() const {
		return Lock(this);
	}
};
