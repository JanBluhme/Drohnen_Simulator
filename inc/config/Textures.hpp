#pragma once

#include "textures/logos/hspf_logo.png.hpp"
#include "textures/astro/X_2k_earth_clouds.jpg.hpp"
#include "textures/astro/X_2k_earth_daymap.jpg.hpp"
#include "textures/astro/X_2k_earth_nightmap.jpg.hpp"
#include "textures/astro/X_2k_jupiter.jpg.hpp"
#include "textures/astro/X_2k_mars.jpg.hpp"
#include "textures/astro/X_2k_mercury.jpg.hpp"
#include "textures/astro/X_2k_moon.jpg.hpp"
#include "textures/astro/X_2k_neptune.jpg.hpp"
#include "textures/astro/X_2k_saturn.jpg.hpp"
#include "textures/astro/X_2k_stars.jpg.hpp"
#include "textures/astro/X_2k_stars_milky_way.jpg.hpp"
#include "textures/astro/X_2k_sun.jpg.hpp"
#include "textures/astro/X_2k_uranus.jpg.hpp"
#include "textures/astro/X_2k_venus_atmosphere.jpg.hpp"
#include "textures/astro/X_2k_venus_surface.jpg.hpp"
#include "textures/astro/X_8081_earthlights2k.jpg.hpp"
#include "textures/astro/X_8081_earthmap2k.jpg.hpp"
#include "textures/astro/moonmap2k.jpg.hpp"
#include "textures/misc/panel1.jpg.hpp"
#include "textures/misc/panel2.png.hpp"
#include "textures/misc/panel3.jpg.hpp"
#include "textures/misc/stone.jpg.hpp"
#include "textures/misc/robot1.jpg.hpp"
#include "textures/misc/robot2.jpg.hpp"
#include "textures/misc/robot3.jpg.hpp"
#include "textures/misc/smiley_gray.jpg.hpp"
#include "textures/misc/smiley_neptune.jpg.hpp"
#include "textures/misc/smiley_neptune_gray.jpg.hpp"


#include <cstddef>

namespace robo {
namespace config {

struct TextureConfig {
	bool              isInMemory;
	char const* const nameOrData;
	std::size_t       size{0};

	static auto make_from(std::string_view mem)
		-> TextureConfig
	{
		return {true, mem.begin(), mem.size()};
	}
};

struct Textures {
	inline static TextureConfig X_2k_earth_clouds     = TextureConfig::make_from(textures::astro::X_2k_earth_clouds    ());
	inline static TextureConfig X_2k_earth_daymap     = TextureConfig::make_from(textures::astro::X_2k_earth_daymap    ());
	inline static TextureConfig X_2k_earth_nightmap   = TextureConfig::make_from(textures::astro::X_2k_earth_nightmap  ());
	inline static TextureConfig X_2k_jupiter          = TextureConfig::make_from(textures::astro::X_2k_jupiter         ());
	inline static TextureConfig X_2k_mars             = TextureConfig::make_from(textures::astro::X_2k_mars            ());
	inline static TextureConfig X_2k_mercury          = TextureConfig::make_from(textures::astro::X_2k_mercury         ());
	inline static TextureConfig X_2k_moon             = TextureConfig::make_from(textures::astro::X_2k_moon            ());
	inline static TextureConfig X_2k_neptune          = TextureConfig::make_from(textures::astro::X_2k_neptune         ());
	inline static TextureConfig X_2k_saturn           = TextureConfig::make_from(textures::astro::X_2k_saturn          ());
	inline static TextureConfig X_2k_stars            = TextureConfig::make_from(textures::astro::X_2k_stars           ());
	inline static TextureConfig X_2k_stars_milky_way  = TextureConfig::make_from(textures::astro::X_2k_stars_milky_way ());
	inline static TextureConfig X_2k_sun              = TextureConfig::make_from(textures::astro::X_2k_sun             ());
	inline static TextureConfig X_2k_uranus           = TextureConfig::make_from(textures::astro::X_2k_uranus          ());
	inline static TextureConfig X_2k_venus_atmosphere = TextureConfig::make_from(textures::astro::X_2k_venus_atmosphere());
	inline static TextureConfig X_2k_venus_surface    = TextureConfig::make_from(textures::astro::X_2k_venus_surface   ());
	inline static TextureConfig X_8081_earthlights2k  = TextureConfig::make_from(textures::astro::X_8081_earthlights2k ());
	inline static TextureConfig X_8081_earthmap2k     = TextureConfig::make_from(textures::astro::X_8081_earthmap2k    ());
	inline static TextureConfig moonmap2k             = TextureConfig::make_from(textures::astro::moonmap2k            ());

	inline static TextureConfig stone                 = TextureConfig::make_from(textures::misc::stone                 ());
	inline static TextureConfig panel1                = TextureConfig::make_from(textures::misc::panel1                ());
	inline static TextureConfig panel2                = TextureConfig::make_from(textures::misc::panel2                ());
	inline static TextureConfig panel3                = TextureConfig::make_from(textures::misc::panel3                ());
	inline static TextureConfig robot1                = TextureConfig::make_from(textures::misc::robot1                ());
	inline static TextureConfig robot2                = TextureConfig::make_from(textures::misc::robot2                ());
	inline static TextureConfig robot3                = TextureConfig::make_from(textures::misc::robot3                ());
	inline static TextureConfig smiley_gray           = TextureConfig::make_from(textures::misc::smiley_gray           ());
	inline static TextureConfig smiley_neptune        = TextureConfig::make_from(textures::misc::smiley_neptune        ());
	inline static TextureConfig smiley_neptune_gray   = TextureConfig::make_from(textures::misc::smiley_neptune_gray   ());
};

} // namespace config
} // namespace robo
