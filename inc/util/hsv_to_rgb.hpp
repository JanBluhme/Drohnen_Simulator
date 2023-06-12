#pragma once
#include <cmath>
#include "math/Vertex.hpp"

inline Vertex<double,3> hsv_to_rgb(
	  double hue
	, double saturation
	, double value
) {
	double C = value * saturation;//croma;
	double m = value - C;
	double H = fmod( hue*3.0/M_PI, 6.0 );
	double X = C*(1.0 - fabs( fmod(H, 2.0) - 1.0 ) );
	double r, g, b;
	switch(static_cast<unsigned char>(H)) {
		case 0: r = C; g = X; b = 0; break;
		case 1: r = X; g = C, b = 0; break;
		case 2: r = 0; g = C, b = X; break;
		case 3: r = 0; g = X, b = C; break;
		case 4: r = X; g = 0, b = C; break;
		case 5: r = C; g = 0, b = X; break;
		default:r = 0; g = 0; b = 0;
	}
	return {r + m, g + m, b + m};
}
