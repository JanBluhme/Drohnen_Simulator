#pragma once
#include <array>
#include <type_traits>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <vector>
#include <concepts>

namespace detail {

template <class T>
concept Integer =
	    std::is_integral<T>::value
	&& !std::is_same_v<T, bool>
;

template<typename buffer_t, Integer integer_t>
struct SimpleSerializationBufferBase {
	using unsigned_t = std::make_unsigned_t<integer_t>;
	using signed_t   = std::make_signed_t<integer_t>;

	buffer_t&  _data;
	unsigned_t _count{};

	SimpleSerializationBufferBase(buffer_t& buffer)
		: _data{buffer}
	{}

	unsigned_t& count() {
		return _count;
	}
	unsigned_t count() const {
		return _count;
	}
	auto begin() const {
		return std::begin(_data);
	}
	auto begin() {
		return std::begin(_data);
	}
	auto end() const {
		return std::next(begin(), static_cast<signed_t>(count()));
	}
	auto end() {
		return std::next(begin(), static_cast<signed_t>(count()));
	}
	auto data() const {
		return std::data(_data);
	}

	auto available() const {
		return static_cast<unsigned_t>(std::distance(std::end(_data), end()));
	}
	struct ResetPoint {
		SimpleSerializationBufferBase& parent;
		unsigned_t                     count;

		void apply() {
			parent.count() = count;
		}
	};
	ResetPoint reset_point() {
		return {*this, count()};
	}
};

template<typename buffer_t, Integer integer_t>
struct SimpleDeserializationBufferBase {
	using unsigned_t = std::make_unsigned_t<integer_t>;
	using signed_t   = std::make_signed_t<integer_t>;

	buffer_t&  _data;
	unsigned_t _bytes_total{};
	unsigned_t _available{};

	SimpleDeserializationBufferBase(buffer_t& buffer)
		: _data{buffer}
	{}
	unsigned_t bytes_total() const {
		return _bytes_total;
	}
	unsigned_t& bytes_total() {
		return _bytes_total;
	}
	unsigned_t available() const {
		return _available;
	}
	unsigned_t& available() {
		return _available;
	}
	auto begin() const {
		return std::next(end(), -static_cast<signed_t>(_available));
	}
	auto end() const {
		return std::next(data(), static_cast<signed_t>(bytes_total()));
	}
	auto data() const {
		return std::data(_data);
	}
	auto data() {
		return std::data(_data);
	}
	struct ResetPoint {
		SimpleDeserializationBufferBase& parent;
		unsigned_t                       available;

		void apply() {
			parent._available = available;
		}
	};
	ResetPoint reset_point() {
		return {*this, _available};
	}
};

template<std::size_t N, typename buffer_t, Integer integer_t>
struct FixedSerializationBufferBase
	: SimpleSerializationBufferBase<buffer_t, integer_t>
{
	using unsigned_t = std::make_unsigned_t<integer_t>;

	buffer_t& buffer;

	FixedSerializationBufferBase(buffer_t buffer)
		: SimpleSerializationBufferBase<buffer_t, integer_t>{buffer}
		, buffer{buffer}
	{}

	void reset() {
		this->count() = 0;
	}

	bool insert(void const* bytes, unsigned_t count) {
		if(this->available() < count) {
			return false;
		}
		auto d = this->end();
		this->count() += count;
		std::memcpy(std::addressof(*d), bytes, count);
		return true;
	}
};

template<std::size_t N, typename buffer_t, Integer integer_t>
struct FixedDeserializationBufferBase
	: SimpleDeserializationBufferBase<buffer_t, integer_t>
{
	using unsigned_t = std::make_unsigned_t<integer_t>;

	buffer_t& buffer;

	FixedDeserializationBufferBase(buffer_t& buffer)
		: SimpleDeserializationBufferBase<buffer_t, integer_t>{buffer}
		, buffer{buffer}
	{}

	void reset(unsigned_t count) {
		this->available()   = count;
		this->bytes_total() = count;
	}

	bool extract(void* bytes, unsigned_t count) {
		if(this->available() < count) {
			return false;
		}
		auto b = this->begin();
		this->available() -= count;
		std::memcpy(bytes, std::addressof(*b), count);
		return true;
	}
};

template<typename buffer_t, Integer integer_t>
struct DynamicSerializationBufferBase
	: SimpleSerializationBufferBase<buffer_t, integer_t>
{
	using unsigned_t = std::make_unsigned_t<integer_t>;

	buffer_t& buffer;

	DynamicSerializationBufferBase(buffer_t& buffer)
		: SimpleSerializationBufferBase<buffer_t, integer_t>{buffer}
		, buffer{buffer}
	{
		reset();
	}

	void reset() {
		buffer.clear();
		this->count() = 0;
	}

	bool insert(void const* bytes, unsigned_t count) {
		if(this->available() < count) {
			buffer.resize(this->count() + count);
		}
		auto d = this->end();
		this->count() += count;
		std::memcpy(std::addressof(*d), bytes, count);
		return true;
	}
};

template<typename buffer_t, Integer integer_t>
struct DynamicDeserializationBufferBase
	: SimpleDeserializationBufferBase<buffer_t, integer_t>
{
	using unsigned_t = std::make_unsigned_t<integer_t>;

	buffer_t& buffer;

	DynamicDeserializationBufferBase(buffer_t& buffer)
		: SimpleDeserializationBufferBase<buffer_t, integer_t>{buffer}
		, buffer{buffer}
	{}
	void reset(unsigned_t count) {
		buffer.resize(count);
		this->available()   = count;
		this->bytes_total() = count;
	}
	bool extract(void* bytes, unsigned_t count) {
		if(this->available() < count) {
			return false;
		}
		auto b = this->begin();
		this->available() -= count;
		std::memcpy(bytes, std::addressof(*b), count);
		return true;
	}
};

}   // namespace detail

template<typename storage_t = std::byte, detail::Integer integer_t = std::size_t>
struct DynamicSerializationBuffer
	: detail::DynamicSerializationBufferBase<std::vector<storage_t>, integer_t>
{
	using buffer_t = std::vector<storage_t>;
	buffer_t buffer;

	DynamicSerializationBuffer()
		: detail::DynamicSerializationBufferBase<buffer_t, integer_t>{buffer}
	{}
};

template<typename storage_t = std::byte, detail::Integer integer_t = std::size_t>
struct DynamicDeserializationBuffer
	: detail::DynamicDeserializationBufferBase<std::vector<storage_t>, integer_t>
{
	using buffer_t = std::vector<storage_t>;
	buffer_t buffer;

	DynamicDeserializationBuffer()
		: detail::DynamicDeserializationBufferBase<buffer_t, integer_t>{buffer}
	{}
};

template<typename buffer_t, detail::Integer integer_t = std::size_t>
struct DynamicSerializationBuffer_view
	: detail::DynamicSerializationBufferBase<buffer_t, integer_t>
{
	DynamicSerializationBuffer_view(buffer_t& buffer)
		: detail::DynamicSerializationBufferBase<buffer_t, integer_t>{buffer}
	{}
};

template<typename buffer_t, detail::Integer integer_t = std::size_t>
struct DynamicDeserializationBuffer_view
	: detail::DynamicDeserializationBufferBase<buffer_t, integer_t>
{
	DynamicDeserializationBuffer_view(buffer_t& buffer)
		: detail::DynamicDeserializationBufferBase<buffer_t, integer_t>{buffer}
	{}
};
