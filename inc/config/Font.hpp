#pragma once
#include "fonts/Ubuntu_M.ttf.hpp"
#include "fonts/Ubuntu_B.ttf.hpp"

#include <cstddef>

namespace robo {
namespace config {

struct FontConfig {
	bool              isInMemory;
	char const* const nameOrData;
	std::size_t       size{0};

	static auto make_from(std::string_view mem)
		-> FontConfig
	{
		return {true, mem.begin(), mem.size()};
	}
};


struct Font {
	constexpr static FontConfig ubuntu_m_file{false, "fonts/Ubuntu-M.ttf"};
	constexpr static FontConfig ubuntu_b_file{false, "fonts/Ubuntu-B.ttf"};
	inline static FontConfig ubuntu_m = FontConfig::make_from(fonts::Ubuntu_M());
	inline static FontConfig ubuntu_b = FontConfig::make_from(fonts::Ubuntu_B());
};

} // namespace config
} // namespace robo
