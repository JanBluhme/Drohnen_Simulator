#pragma once
#include "simple_gl/MeshData.hpp"
#include <memory>
#include <limits>

template<bool has_texture, bool has_color>
auto make_square_xy(
	  double                  width
	, double                  height
	, double                  z
	, Vertex<double,4> const& color = {1,1,1,1}
)
	-> MeshData<true,has_texture,has_color>
{
	using MeshVertex = MeshVertex<double, true,has_texture,has_color>;
	using V3 = Vertex<double, 3>;
	using V2 = Vertex<double, 2>;
	auto make_quad_vertices_z = [&]() {
		std::vector<MeshVertex> vertices(4);
		vertices[0].position = V3{ width/2, -height/2, z}; //B
		vertices[1].position = V3{ width/2,  height/2, z}; //C
		vertices[2].position = V3{-width/2,  height/2, z}; //D
		vertices[3].position = V3{-width/2, -height/2, z}; //A
		vertices[0].normal   = V3{0.0, 0.0, 1.0};
		vertices[1].normal   = vertices[0].normal;
		vertices[2].normal   = vertices[0].normal;
		vertices[3].normal   = vertices[0].normal;
		if constexpr (has_texture) {
			vertices[0].tex      = V2{1.0, 1.0};
			vertices[1].tex      = V2{1.0, 0.0};
			vertices[2].tex      = V2{0.0, 0.0};
			vertices[3].tex      = V2{0.0, 1.0};
		}
		if constexpr (has_color) {
			vertices[0].color = color;
			vertices[1].color = color;
			vertices[2].color = color;
			vertices[3].color = color;
		}
		return vertices;
	};
	auto make_quad_indices = []() {
		return std::vector<unsigned int>{0,1,2,0,2,3};
	};
	return MeshData<true,has_texture,has_color>{make_quad_vertices_z(), make_quad_indices()};
}

template<bool has_texture, bool has_color>
auto make_simple_disk(
	  double           radius
	, double           azimuth_0
	, double           azimuth_1
	, std::size_t      sectors
	, Vertex<double,4> color = {1.0, 1.0, 1.0, 1.0}
)
	-> MeshData<true,has_texture,has_color>
{
	using mesh_data_t = MeshData<true,has_texture,has_color>;
	using MeshVertex = MeshVertex<double, true,has_texture,has_color>;
	using V3 = Vertex<double,3>;
	using V2 = Vertex<double,2>;

	auto make_disc_vertices = [&]() {
		std::vector<MeshVertex> vertices(sectors + 2);
		vertices[0].position = V3{0.0, 0.0, 0.0};
		vertices[0].normal   = V3{0.0, 0.0, 1.0};
		if constexpr(has_texture) {
			vertices[0].tex      = V2{0.5, 0.0};
		}
		if constexpr(has_color) {
			vertices[0].color = color;
		}
		for(std::size_t i = 0; i <= sectors; ++i) {
			double phi = azimuth_0 + i * (azimuth_1 - azimuth_0) / sectors;
			Vertex<double,2> v   = polar(1.0, phi);
			if constexpr (has_texture) {
				vertices[1 + i].tex      = V2{(double)i/sectors, 1.0};
			}
			if constexpr (has_color) {
				vertices[1 + i].color    = color;
			}
			vertices[1 + i].position = V3{v[0]*radius, v[1]*radius, 0.0};
			vertices[1 + i].normal   = vertices[0].normal;
		}
		return vertices;
	};
	auto make_disc_indices = [&]() {
		std::vector<unsigned int> indices(3*sectors);
		for(std::size_t i = 0; i < sectors; ++i) {
			indices[3*i + 0] = 0;
			indices[3*i + 1] = 1 + i;
			indices[3*i + 2] = 1 + (i + 1);
		}
		return indices;
	};
	return mesh_data_t{make_disc_vertices(), make_disc_indices()};
}

template<bool has_texture, bool has_color>
auto make_simple_ring(
	  double           radius0
	, double           radius1
	, double           azimuth_0
	, double           azimuth_1
	, std::size_t      sectors
	, Vertex<double,4> color = {1.0, 1.0, 1.0, 1.0}
)
	-> MeshData<true,has_texture,has_color>
{
	using mesh_data_t = MeshData<true,has_texture,has_color>;
	using MeshVertex = MeshVertex<double, true,has_texture,has_color>;
	using V3 = Vertex<double,3>;
	using V2 = Vertex<double,2>;

	auto make_disc_vertices = [&](double radius) {
		std::vector<MeshVertex> vertices(sectors + 1);
		for(std::size_t i = 0; i <= sectors; ++i) {
			double phi = azimuth_0 + i * (azimuth_1 - azimuth_0) / sectors;
			Vertex<double,2> v   = polar(1.0, phi);
			if constexpr (has_texture) {
				vertices[i].tex      = V2{(double)i/sectors, 1.0};
			}
			if constexpr (has_color) {
				vertices[i].color    = color;
			}
			vertices[i].position = V3{v[0]*radius, v[1]*radius, 0.0};
			vertices[i].normal   = vertices[0].normal;
		}
		return vertices;
	};
	auto make_ring_vertices = [&]() {
		std::vector<MeshVertex> inner = make_disc_vertices(radius0);
		std::vector<MeshVertex> outer = make_disc_vertices(radius1);
		inner.insert(inner.end(), outer.begin(), outer.end());
		return inner;
	};
	auto make_ring_indices = [&]() {
		std::vector<unsigned int> indices(6*sectors);
		for(std::size_t i = 0; i < sectors; ++i) {
			indices[6*i + 0] = i;
			indices[6*i + 1] = i + 1;
			indices[6*i + 2] = sectors + i + 2;
			indices[6*i + 3] = i;
			indices[6*i + 4] = sectors + i + 1;
			indices[6*i + 5] = sectors + i + 2;
		}
		return indices;
	};
	return mesh_data_t{make_ring_vertices(), make_ring_indices()};
}

template<bool has_texture, bool has_color>
auto make_simple_cube(
	  double                      wx    = 1.0
	, double                      wy    = 1.0
	, double                      wz    = 1.0
	, Vertex<double,4> const& color = {1,1,1,1}
)
	-> MeshData<true,has_texture,has_color>
{
	using mesh_data_t = MeshData<true,has_texture,has_color>;
	auto make_square_pair = [down = Transform::rotate_y(M_PI)](double w, double h, double z) {
		mesh_data_t result = make_square_xy<has_texture, has_color>(w, h, z);
		result += down * result;
		return result;
	};
	Transform rotX = Transform::rotate_x(M_PI_2);
	Transform rotY = Transform::rotate_y(M_PI_2);
	mesh_data_t result;
	result +=               make_square_pair(wx, wy, wz / 2.0);
	result += rotX * rotY * make_square_pair(wy, wz, wx / 2.0);
	result +=        rotX * make_square_pair(wx, wz, wy / 2.0);
	return result;
}

inline auto make_default_color_gen(Vertex<double, 4> const& color) {
	return [=](double, double) {
		return color;
	};
}
inline auto make_default_tex_gen(double min_x, double min_y, double max_x, double max_y) {
	double delta_x = max_x - min_x;
	double delta_y = max_y - min_y;
	return [min_x, min_y, delta_x, delta_y](double x, double y) {
		return Vertex<double, 2>{
			  (x - min_x) / delta_x
			, (y - min_y) / delta_y
		};
	};
}

// creates something with respect to spere coordinates
template<bool has_texture, bool has_color>
auto make_spherical_mesh(
	    double      height_0
	  , double      height_1
	  , double      azimuth_0
	  , double      azimuth_1
	  , std::size_t stacks
	  , std::size_t sectors
	  , auto&&      gen_position
	  , auto&&      gen_normal
	  , auto&&      gen_tex   = [](double, double) { return Vertex<double, 2>{}; }
	  , auto&&      gen_color = make_default_color_gen(Vertex<double,4>{})
)
	-> MeshData<true,has_texture,has_color>
	   requires                                                             /* azimuth, height */
			   std::is_invocable_r_v<Vertex<double,3>, decltype(gen_position), double, double>
			&& std::is_invocable_r_v<Vertex<double,3>, decltype(gen_normal  ), double, double>
			&& std::is_invocable_r_v<Vertex<double,2>, decltype(gen_tex     ), double, double>
			&& std::is_invocable_r_v<Vertex<double,4>, decltype(gen_color   ), double, double>
{
	using MeshVertex = MeshVertex<double, true,has_texture,has_color>;
	auto make_vertices = [&]() {
		std::vector<MeshVertex> vertices;
		vertices.reserve((stacks + 1) * (sectors + 1));
		double const d_height  = (height_1 - height_0) / stacks;
		double const d_azimuth = (azimuth_1 - azimuth_0) / sectors;
		for(std::size_t i = 0; i <= stacks; ++i) {
			double const height = height_0 + i * d_height;
			for(std::size_t j = 0; j <= sectors; ++j) {
				double const azimuth = azimuth_0 + j * d_azimuth;
				MeshVertex vertex;
				vertex.position = gen_position(azimuth, height);
				vertex.normal   = gen_normal(  azimuth, height);
				if constexpr (has_texture) {
					vertex.tex = gen_tex(azimuth, height);
				}
				if constexpr (has_color) {
					vertex.color = gen_color(azimuth, height);
				}
				vertices.push_back(vertex);
			}
		}
		return vertices;
	};
	auto make_indices = [&]() {
		std::vector<unsigned int> indices;
		indices.reserve(stacks * sectors * 6);
		for(std::size_t i = 0; i < stacks; ++i) {
			unsigned short k1 = i * (sectors + 1);     // beginning of current stack
			unsigned short k2 = k1 + sectors + 1;      // beginning of next stack

			for(std::size_t j = 0; j < sectors; ++j, ++k1, ++k2) {
				indices.push_back(k1);
				indices.push_back(k2);
				indices.push_back(k1 + 1);

				indices.push_back(k1 + 1);
				indices.push_back(k2);
				indices.push_back(k2 + 1);
			}
		}
		return indices;
	};
	return MeshData<true,has_texture,has_color>{make_vertices(), make_indices()};
}

template<bool has_texture, bool has_color>
auto make_simple_sherical_segment(
	  double                      radius
	, double                      height_0 // angle
	, double                      height_1
	, double                      azimuth_0 // angle
	, double                      azimuth_1
	, std::size_t                 stacks  = 24
	, std::size_t                 sectors = 24
	, Vertex<double,4> const& color   = {1,1,1,1}
)
	-> MeshData<true,has_texture,has_color>
{
	auto gen_position = [&](double azimuth, double height) {
		return Vertex<double, 3> {
			  radius * std::cos(height) * std::cos(azimuth)
			, radius * std::cos(height) * std::sin(azimuth)
			, radius * std::sin(height)
		};
	};
	auto gen_normal = [&](double azimuth, double height) {
		return gen_position(height, azimuth) / radius;
	};
	return make_spherical_mesh<has_texture, has_color>(
	    height_0
	  , height_1
	  , azimuth_0
	  , azimuth_1
	  , stacks
	  , sectors
	  , gen_position
	  , gen_normal
	  , make_default_tex_gen(azimuth_0, height_0, azimuth_1, height_1)
	  , make_default_color_gen(color)
	);
}

template<bool has_texture, bool has_color>
auto make_simple_sherical_segment_radial_offset(
	  double                      radius
	, double                      radial_offset
	, double                      height_0 // angle
	, double                      height_1
	, double                      azimuth_0 // angle
	, double                      azimuth_1
	, std::size_t                 stacks  = 24
	, std::size_t                 sectors = 24
	, Vertex<double,4> const& color   = {1,1,1,1}
)
	-> MeshData<true,has_texture,has_color>
{
	auto gen_position = [&](double azimuth, double height) {
		return Vertex<double, 3> {
			  radial_offset * std::cos(azimuth) + radius * std::cos(height) * std::cos(azimuth)
			, radial_offset * std::sin(azimuth) + radius * std::cos(height) * std::sin(azimuth)
			,                                     radius * std::sin(height)
		};
	};
	auto gen_normal = [&](double azimuth, double height) {
		return Vertex<double, 3> {
			  std::cos(height) * std::cos(azimuth)
			, std::cos(height) * std::sin(azimuth)
			, std::sin(height)
		};
	};
	return make_spherical_mesh<has_texture, has_color>(
	    height_0
	  , height_1
	  , azimuth_0
	  , azimuth_1
	  , stacks
	  , sectors
	  , gen_position
	  , gen_normal
	  , make_default_tex_gen(azimuth_0, height_0, azimuth_1, height_1)
	  , make_default_color_gen(color)
	);
}

template<bool has_texture, bool has_color>
auto make_simple_shere(
	  double                      radius  = 1.0
	, std::size_t                 stacks  = 24
	, std::size_t                 sectors = 24
	, Vertex<double,4> const& color   = {1,1,1,1}
)
	-> MeshData<true,has_texture,has_color>
{
	return make_simple_sherical_segment<has_texture, has_color>(
		  radius
		,-M_PI_2
		, M_PI_2
		, 0.0
		, 2.0 * M_PI
		, stacks
		, sectors
		, color
	);
}

template<bool has_texture, bool has_color>
auto make_simple_cone_segment(
	  double radius0, double height0
	, double radius1, double height1
	, double azimuth_0
	, double azimuth_1
	, std::size_t sectors
	, bool show_disc_0 = true
	, bool show_disc_1 = true
	, Vertex<double,4> const& color = {1,1,1,1} /** sorry for horrible interface ... */
)
	-> MeshData<true,has_texture,has_color>
{
	using mesh_data_t = MeshData<true,has_texture,has_color>;
	using MeshVertex = MeshVertex<double, true,has_texture,has_color>;
	using V3 = Vertex<double, 3>;
	using V2 = Vertex<double, 2>;
// 	auto make_disc_vertices = [&](bool is_up) {
// 		double h   = is_up ? height1 : height0;
// 		double r   = is_up ? radius1 : radius0;
// 		double nz  = is_up ? 1.0 : -1.0;
// 		double ty0 = is_up ? 0.0 : 1.0;
// 		double ty1 = 1 - ty0;
// 		std::vector<MeshVertex> vertices(sectors + 2);
// 		vertices[0].position = V3{0.0, 0.0, h};
// 		vertices[0].normal   = V3{0.0, 0.0, nz};
// 		if constexpr(has_texture) {
// 			vertices[0].tex      = V2{0.5, ty0};
// 		}
// 		if constexpr(has_color) {
// 			vertices[0].color = color;
// 		}
// 		for(std::size_t i = 0; i <= sectors; ++i) {
// 			double phi = azimuth_0 + i * (azimuth_1 - azimuth_0) / sectors;
// 			Vertex<double,2> v   = polar(1.0, phi);
// 			if constexpr (has_texture) {
// 				vertices[1 + i].tex      = V2{(double)i/sectors, ty1};
// 			}
// 			if constexpr (has_color) {
// 				vertices[1 + i].color    = color;
// 			}
// 			vertices[1 + i].position = V3{v[0]*r, v[1]*r, h};
// 			vertices[1 + i].normal   = vertices[0].normal;
// 		}
// 		return vertices;
// 	};
// 	auto make_disc_indices = [&]() {
// 		std::vector<unsigned int> indices(3*sectors);
// 		for(std::size_t i = 0; i < sectors; ++i) {
// 			indices[3*i + 0] = 0;
// 			indices[3*i + 1] = 1 + i;
// 			indices[3*i + 2] = 1 + (i + 1);
// 		}
// 		return indices;
// 	};
	auto make_open_cylinder_vertices = [&]() {
		std::vector<MeshVertex> vertices(2*(sectors + 1));
		for(std::size_t i = 0; i <= sectors; ++i) {
			double phi = azimuth_0 + i * (azimuth_1 - azimuth_0) / sectors;
			V2 v = polar(1.0, phi);
			double dz = (radius1 - radius0) / (height1 - height0);
			if constexpr (has_texture) {
				vertices[i].tex      = V2{(double)i/sectors, 0.0};
			}
			if constexpr (has_color) {
				vertices[i].color    = color;
			}
			vertices[i].position = V3{v[0]*radius0, v[1]*radius0, height0};
			vertices[i].normal   = V3{v[0], v[1], dz}.normalize();
			if constexpr (has_texture) {
				vertices[i + sectors + 1].tex      = V2{(double)i/sectors, 1.0};
			}
			if constexpr (has_color) {
				vertices[i + sectors + 1].color    = color;
			}
			vertices[i + sectors + 1].position = V3{v[0]*radius1, v[1]*radius1, height1};
			vertices[i + sectors + 1].normal   = vertices[i].normal;
		}
		return vertices;
	};
	auto make_open_cylinder_indices = [&]() {
		std::vector<unsigned int> indices;
		unsigned short k1 = 0;     // beginning of current stack
		unsigned short k2 = k1 + sectors + 1;      // beginning of next stack

		for(std::size_t j = 0; j < sectors; ++j, ++k1, ++k2) {
			indices.push_back(k1);
			indices.push_back(k2);
			indices.push_back(k1 + 1);

			indices.push_back(k1 + 1);
			indices.push_back(k2);
			indices.push_back(k2 + 1);
		}
		return indices;
	};
	mesh_data_t result{make_open_cylinder_vertices(), make_open_cylinder_indices()};
	if(show_disc_0) {
		result += Transform::translate({0.0, 0.0, height0}) * Transform::rotate_y(M_PI) * make_simple_disk<has_texture, has_color>(radius0, azimuth_0, azimuth_1, sectors);
		//result += mesh_data_t{make_disc_vertices(false), make_disc_indices()};
	}
	if(show_disc_1) {
		result += Transform::translate({0.0, 0.0, height1}) * make_simple_disk<has_texture, has_color>(radius1, azimuth_0, azimuth_1, sectors);
		//result += mesh_data_t{make_disc_vertices(true), make_disc_indices()};
	}
	return result;
}

template<bool has_texture, bool has_color>
auto make_simple_cone(
	  double radius0, double height0
	, double radius1, double height1
	, std::size_t points
	, bool show_disc_0 = true
	, bool show_disc_1 = true
	, Vertex<double,4> const& color = {1,1,1,1} /** sorry for horrible interface ... */
)
	-> MeshData<true,has_texture,has_color>
{
	return make_simple_cone_segment<has_texture, has_color>(
		  radius0
		, height0
		, radius1
		, height1
		, 0.0
		, 2.0 * M_PI
		, points
		, show_disc_0
		, show_disc_1
		, color
	);
}

template<bool has_texture, bool has_color>
auto make_simple_cylinder(
	  double radius
	, double height0, double height1
	, std::size_t points
	, bool show_disc_0 = true
	, bool show_disc_1 = true
	, Vertex<double,4> const& color = {1,1,1,1}
)
	-> MeshData<true,has_texture,has_color>
{
	return make_simple_cone<has_texture, has_color>(
		  radius
		, height0
		, radius
		, height1
		, points
		, show_disc_0
		, show_disc_1
		, color
	);
}

template<bool has_texture, bool has_color>
auto make_simple_grown_cube(
	  double                      wx      = 1.0
	, double                      wy      = 1.0
	, double                      wz      = 1.0
	, double                      radius  = 0.5
	, std::size_t                 stacks  = 12
	, std::size_t                 sectors = 12
	, Vertex<double,4> const& color = {1,1,1,1}
)
	-> MeshData<true,has_texture,has_color>
{
	using mesh_data_t = MeshData<true,has_texture,has_color>;
	auto translate = [](double x, double y, double z) {
		return Transform::translate({x, y, z});
	};
	auto sub_sphere = [&](double height1, double azimuth_0) {
		return make_simple_sherical_segment<has_texture, has_color>(
			  radius
			, 0.0
			, height1
			, azimuth_0
			, azimuth_0 + M_PI_2
			, stacks
			, sectors
		);
	};
	auto sub_cylinder = [&](double length, double azimuth_0) {
		return make_simple_cone_segment<has_texture, has_color>(
			  radius
			, -length / 2.0
			, radius
			, length / 2.0
			, azimuth_0
			, azimuth_0 + M_PI_2
			, sectors
			, false
			, false
		);
	};
	auto sub_sphere_north = [&](double azimuth_0) {
		return sub_sphere( M_PI_2, azimuth_0);
	};
	auto sub_sphere_south = [&](double azimuth_0) {
		return sub_sphere(-M_PI_2, azimuth_0);
	};
	auto sub_cylinder_x = [&](double azimuth_0) {
		return sub_cylinder(wx, azimuth_0);
	};
	auto sub_cylinder_y = [&](double azimuth_0) {
		return sub_cylinder(wy, azimuth_0);
	};
	auto sub_cylinder_z = [&](double azimuth_0) {
		return sub_cylinder(wz, azimuth_0);
	};
	auto four_pieces_at = [&](double x, double y, double z, auto& gen) {
		mesh_data_t r;
		r += translate( x,  y, z) * gen(0.0 * M_PI_2);
		r += translate(-x,  y, z) * gen(1.0 * M_PI_2);
		r += translate(-x, -y, z) * gen(2.0 * M_PI_2);
		r += translate( x, -y, z) * gen(3.0 * M_PI_2);
		return r;
	};
	mesh_data_t result;
	Transform rotX = Transform::rotate_x(M_PI_2);
	Transform rotY = Transform::rotate_y(M_PI_2);
	double wx2 = wx / 2.0;
	double wy2 = wy / 2.0;
	double wz2 = wz / 2.0;

	auto make_square_pair = [down = Transform::rotate_y(M_PI)](double w, double h, double z) {
		mesh_data_t result = make_square_xy<has_texture, has_color>(w, h, z);
		result += down * result;
		return result;
	};

	result +=               make_square_pair(wx, wy, wz2 + radius);
	result += rotX * rotY * make_square_pair(wy, wz, wx2 + radius);
	result +=        rotX * make_square_pair(wx, wz, wy2 + radius);

	result += four_pieces_at(wx2, wy2,  wz2, sub_sphere_north);
	result += four_pieces_at(wx2, wy2, -wz2, sub_sphere_south);
	result +=        four_pieces_at(wx2, wy2, 0.0, sub_cylinder_z);
	result += rotY * four_pieces_at(wz2, wy2, 0.0, sub_cylinder_x);
	result += rotX * four_pieces_at(wx2, wz2, 0.0, sub_cylinder_y);
	return result;
}

template<bool has_texture, bool has_color>
auto make_simple_grown_cylinder(
	  double radius
	, double height0, double height1
	, double grow_radius
	, std::size_t sectors
	, std::size_t stacks
	, Vertex<double,4> const& color = {1,1,1,1}
)
	-> MeshData<true,has_texture,has_color>
{
	using mesh_data_t = MeshData<true,has_texture,has_color>;

	mesh_data_t result;
	result += make_simple_cylinder<has_texture, has_color>(radius + grow_radius, height0, height1, sectors, false, false, color);
	mesh_data_t cap = make_simple_sherical_segment_radial_offset<has_texture, has_color>(
		  grow_radius
		, radius
		, 0.0 // angle
		, M_PI_2
		, 0.0
		, 2.0 * M_PI
		, stacks
		, sectors
		, color
	);
	cap += Transform::translate({0.0, 0.0, grow_radius}) * make_simple_disk<has_texture, has_color>(radius, 0.0, 2.0 * M_PI, sectors);
	result += Transform::translate({0.0, 0.0, height1}) * cap;
	result += Transform::translate({0.0, 0.0, height0}) * Transform::rotate_y(M_PI) * cap;
	return result;
}
