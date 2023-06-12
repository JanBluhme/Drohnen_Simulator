#pragma once
#include <string>
#include <cstdio>

template<typename... Args>
inline std::string string_format(char const* fmt, Args... args) {
	//https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
	size_t size = snprintf(nullptr, 0, fmt, args...);
	std::string buf;
	buf.reserve(size + 1);
	buf.resize(size);
	snprintf(&buf[0], size + 1, fmt, args...);
	return buf;
}
