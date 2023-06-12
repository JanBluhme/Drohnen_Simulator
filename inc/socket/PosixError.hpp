#pragma once
#include <string>
#include <stdexcept>
#include <cstring>

class PosixError : public std::runtime_error {
	int _error;
public:
	PosixError(const std::string& message, int error_code)
		: std::runtime_error(message + "::" + strerror(error_code))
		, _error(error_code)
	{}
	int error() const {
		return _error;
	}
};
