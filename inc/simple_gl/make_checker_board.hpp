#pragma once
#include "math/Vertex.hpp"
#include "Texture.hpp"
#include <memory>
#include <vector>
#include <algorithm>

inline Texture make_checker_board(
	  GL_Window* parent
	, int num_x, int num_y
	, int width, int height
	, unsigned char alpha
) {
	std::vector<unsigned char> data(width*height*4);
	int sx = (2*width)/num_x;
	int sy = (2*height)/num_y;
	for(int y = 0; y < height; ++y) {
		for(int x = 0; x < width; ++x) {
			unsigned char* p = data.data() + 4*(width*y + x);
			unsigned char v = (x % sx < sx/2) != (y % sy < sy/2)
				? 255
				: 0
			;
			*p++ = v;
			*p++ = v;
			*p++ = v;
			*p++ = alpha;
		}
	}
	return Texture(parent, width, height, data.data());
}

inline Texture make_checker_board(
	  GL_Window const* parent
	, int num_x, int num_y
	, int width, int height
	, Vertex<double, 4> const& colorA
	, Vertex<double, 4> const& colorB
) {
	auto color = [](Vertex<double, 4> const& color) {
		auto conv = [](double x) {
			return static_cast<unsigned char>(std::clamp(x * 256.0, 0.0, 255.0));
		};
		return Vertex<unsigned char, 4> {
			  conv(color[0])
			, conv(color[1])
			, conv(color[2])
			, conv(color[3])
		};
	};
	Vertex<unsigned char, 4> cA = color(colorA);
	Vertex<unsigned char, 4> cB = color(colorB);
	
	std::vector<unsigned char> data(width*height*4);
	int sx = (2*width)/num_x;
	int sy = (2*height)/num_y;
	for(int y = 0; y < height; ++y) {
		for(int x = 0; x < width; ++x) {
			unsigned char* p = data.data() + 4*(width*y + x);
			Vertex<unsigned char, 4> const& c
				= (x % sx < sx/2) != (y % sy < sy/2)
				? cA
				: cB
			;
			*p++ = c[0];
			*p++ = c[1];
			*p++ = c[2];
			*p++ = c[3];
		}
	}
	return Texture(parent, width, height, data.data());
}
