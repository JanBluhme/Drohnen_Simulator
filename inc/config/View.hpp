#pragma once

#include "Font.hpp"
#include "Logo.hpp"

namespace robo {
namespace config {
	
struct View {
	inline    static auto   font        = Font::ubuntu_m;
	constexpr static int    font_size   = 72;
	inline    static auto   logo_school = Logo::hspf;

	constexpr static double label_height    = 0.075 * 2;
	inline    static auto   label_font      = Font::ubuntu_b;
	constexpr static int    label_font_size = 72;
};

} /** namespace config */
} /** namespace robo */
