#pragma once

#include "textures/logos/hspf_logo.png.hpp"

#include <cstddef>

namespace robo {
namespace config {

struct LogoConfig {
	bool              isInMemory;
	char const* const nameOrData;
	std::size_t       size{0};

	static auto make_from(std::string_view mem)
		-> LogoConfig
	{
		return {true, mem.begin(), mem.size()};
	}
};

struct Logo {
	constexpr static LogoConfig hspf_file{false, "hspf_logo.png"};
	inline static LogoConfig hspf = LogoConfig::make_from(textures::logos::hspf_logo());
};

} // namespace config
} // namespace robo
