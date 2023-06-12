#pragma once

namespace robo {
namespace config {

struct Build {
	constexpr static char const* date = __DATE__;
	constexpr static char const* time = __TIME__;
	constexpr static char const* git  =
		#include "../../git.hash"
	;
};

} /** namespace config */
} /** namespace robo */
