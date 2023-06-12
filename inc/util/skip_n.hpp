#pragma once
#include <cstddef>

template<typename I>
I skip_n(I begin, I end, std::size_t N) {
	while(begin != end && N) {
		++begin;
		--N;
	}
	return begin;
}
