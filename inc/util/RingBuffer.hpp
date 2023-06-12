#pragma once
#include <vector>
#include <algorithm>

template<typename T>
class RingBuffer {
private:
	std::size_t    write_index{};
	std::vector<T> buffer;

	std::size_t getIndex(int index) const {
		return mod(index + write_index, buffer.size());
	}

	int mod(int a, int b) const {
		int r = a % b;
		if((a < 0) != (b < 0)) {
			r += b;
		}
		return r;
	}

public:
	RingBuffer(std::size_t ring_size_) {
		resize(ring_size_);
	}

	std::size_t size() const {
		return buffer.size();
	}

	void resize(std::size_t new_ring_size) {
		new_ring_size = std::max(new_ring_size, std::size_t{1});
		if(buffer.size() == 0) {
			buffer.resize(new_ring_size);
			return;
		}
		if(new_ring_size == buffer.size()) {
			return;
		}

		if(new_ring_size > buffer.size()) {
			std::rotate(
				  buffer.begin()
				, buffer.begin() + write_index
				, buffer.end()
			);
			write_index = buffer.size();
		} else {
			int n_rotate = static_cast<int>(write_index) - static_cast<int>(new_ring_size);
			std::rotate(
				  buffer.begin()
				, buffer.begin() + mod(n_rotate, buffer.size())
				, buffer.end()
			);
			write_index = 0;
		}
		buffer.resize(new_ring_size);
	}

	template<typename TT>
	void push(TT&& value) {
		buffer[write_index] = std::forward<TT>(value);
		++write_index;
		write_index %= buffer.size();
	}

	void clear() {
		buffer.clear();
		resize(0);
	};

	void set_values(T const& v) {
		for(auto& vv : buffer) {
			vv = v;
		}
	}

	T const& operator[](int index) {
		return buffer[getIndex(index)];
	}
};
