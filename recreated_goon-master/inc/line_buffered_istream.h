#pragma once
#include <iostream>
#include <streambuf>
#include <string>

class istream_line_buffer
	: public std::streambuf
{
public:
	explicit istream_line_buffer(std::istream& source)
		: source(source)
	{}

protected:
	std::streambuf* setbuf(char_type* s, std::streamsize n) {
		return nullptr;
	}
	int_type overflow(int_type c) {
		return traits_type::eof();
	}
	int sync() {
		return 0;
	}
	int_type underflow() {
		if (gptr() == nullptr || gptr() >= egptr()) {
			if(!std::getline(source, buffer)) {
				return traits_type::eof();
			}
			buffer += '\n';
			setg(buffer.data(), buffer.data(), buffer.data() + buffer.size());
			return traits_type::to_int_type(buffer[0]);
		} else {
			//return traits_type::to_int_type(*inputBuffer_);
			return traits_type::to_int_type(*gptr());
		}
	}
	std::string const& current_line() const {
		return buffer;
	}

private:
	std::istream& source;
	std::string   buffer;
};

class line_buffered_istream
	: private istream_line_buffer
	, public std::istream
{
public:
	explicit line_buffered_istream(std::istream& source)
		: istream_line_buffer(source)
		, std::istream(this)
	{}
	std::string const& current_line() const {
		return istream_line_buffer::current_line();
	}
};
