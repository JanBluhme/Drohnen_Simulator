#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <sstream>

struct CommandLineArguments {
	using const_iterator = typename std::vector<std::string>::const_iterator;
	std::vector<std::string> args;
	
	CommandLineArguments(int argc, char** argv)
		: args(argv, argv + argc)
	{}
	
	template<typename T>
	T get(std::string_view prefix, T const& default_value) const {
		using std::end;
		auto it = find_prefix(prefix);
		if(it != end(args)) {
			std::stringstream ss(it->substr(prefix.size()));
			T value;
			if(ss >> value) {
				return value;
			}
		}
		return default_value;
	}
	
	const_iterator find_prefix(std::string_view prefix) const {
		using std::begin;
		using std::end;
		return std::find_if(
			  begin(args)
			, end(args)
			, [&](std::string_view v) {
				return v.substr(0, prefix.size()) == prefix;
			}
		);
	}
	bool has_prefix(std::string_view prefix) const {
		return find_prefix(prefix) != end(args);
	}
};
