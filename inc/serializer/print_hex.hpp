#pragma once
#include <cstddef>

template<typename OS>
void print_hex(OS& os, char const* begin, char const* end) {
	auto print = [&](unsigned char x) {
		constexpr char digits[] = {
			  '0', '1', '2', '3', '4', '5', '6', '7'
			, '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
		};
		os << digits[(x>>4)&15] << digits[x&15];
	};
	std::size_t N = end - begin;
	os << N << " [";
	if(begin != end) {
		print(*begin);
		++begin;
		N = 1;
		while(begin != end) {
			os << ' ';
			if(N == 8) {
				os << "| ";
				N = 0;
			}
			print(*begin);
			++begin;
			++N;
		}
	}
	os << ']';
}
