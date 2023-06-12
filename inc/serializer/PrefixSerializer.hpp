#pragma once
#include <array>
#include <vector>
#include <set>
#include <map>
#include <tuple>
#include <utility>
#include <string>
#include <optional>
#include <variant>

#include <iostream>

/** implements:
template<template<class>typename Backend>
struct PrefixSerializer {
	template<typename Buffer, typename... Ts>
	static bool serialize(Buffer& buffer, Ts const&... vs);

	template<typename Buffer, typename... Ts>
	static bool deserialize(Buffer& buffer, Ts&... vs);
};
*/

template<template<class>typename Backend>
struct PrefixSerializer;

template<template<class>typename Backend, typename T>
struct _PrefixSerializer {
	template<typename Buffer>
	static bool serialize(Buffer& buffer, T const& v) {
		if constexpr (
			   std::is_integral_v<T>
			|| std::is_floating_point_v<T>
			|| std::is_enum_v<T>
		) {
			return Backend<T>::serialize(buffer, v);
		} else {
			return T::template serialize<PrefixSerializer<Backend>>(buffer, v);
		}
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, T& v) {
		if constexpr (
			   std::is_integral_v<T>
			|| std::is_floating_point_v<T>
			|| std::is_enum_v<T>
		) {
			return Backend<T>::deserialize(buffer, v);
		} else {
			return T::template deserialize<PrefixSerializer<Backend>>(buffer, v);
		}
	}
};

template<template<class> typename Backend, typename Buffer, typename... Ts>
bool pserialize(Buffer& buffer, Ts const&... vs) {
	auto reset_point = buffer.reset_point();
	bool r = (... && _PrefixSerializer<Backend,Ts>::serialize(buffer, vs));
	if(!r) {
		reset_point.apply();
	}
	return r;
}
template<template<class> typename Backend, typename Buffer, typename... Ts>
bool pdeserialize(Buffer& buffer, Ts&... vs) {
	auto reset_point = buffer.reset_point();
	bool r = (... && _PrefixSerializer<Backend,Ts>::deserialize(buffer, vs));
	if(!r) {
		reset_point.apply();
	}
	return r;
}

template<template<class>typename Backend>
struct PrefixSerializer {
	template<typename Buffer, typename... Ts>
	static bool serialize(Buffer& buffer, Ts const&... vs) {
		return pserialize<Backend>(buffer, vs...);
	}

	template<typename Buffer, typename... Ts>
	static bool deserialize(Buffer& buffer, Ts&... vs) {
		return pdeserialize<Backend>(buffer, vs...);
	}
};

template<template<class> typename Backend, typename T>
struct PRangeSerializer {
	template<typename Buffer, typename I>
	static bool pserialize_range(Buffer& buffer, I begin, I end) {
		if constexpr (
			   std::is_integral_v<T>
			|| std::is_floating_point_v<T>
			|| std::is_enum_v<T>
		) {
			return Backend<T>::serialize_range(buffer, begin, end);
		} else {
			while(begin != end) {
				if(!::pserialize<Backend>(buffer, *begin)) {
					return false;
				}
				++begin;
			};
			return true;
		}
	}
	template<typename Buffer, typename I>
	static bool pdeserialize_range(Buffer& buffer, I begin, I end) {
		if constexpr (
			   std::is_integral_v<T>
			|| std::is_floating_point_v<T>
			|| std::is_enum_v<T>
		) {
			return Backend<T>::deserialize_range(buffer, begin, end);
		} else {
			while(begin != end) {
				if(!::pdeserialize<Backend>(buffer, *begin)) {
					return false;
				}
				++begin;
			}
			return true;
		}
	}
};

template<template<class> typename Backend, typename Buffer, typename I>
bool pserialize_range(Buffer& buffer, I begin, I end) {
	return PRangeSerializer<Backend, decltype(*begin)>::pserialize_range(buffer, begin, end);
}
template<template<class> typename Backend, typename Buffer, typename I>
bool pdeserialize_range(Buffer& buffer, I begin, I end) {
	return PRangeSerializer<Backend, decltype(*begin)>::pdeserialize_range(buffer, begin, end);
}

template<template<class> typename Backend, typename T, std::size_t N>
struct _PrefixSerializer<Backend, T[N]> {
	template<typename Buffer>
	static bool serialize(Buffer& buffer, T const(&v)[N]) {
		return pserialize_range<Backend>(buffer, std::begin(v), std::end(v));
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, T (&v)[N]) {
		return pdeserialize_range<Backend>(buffer, std::begin(v), std::end(v));
	}
};
template<template<class> typename Backend, typename T, std::size_t N>
struct _PrefixSerializer<Backend, std::array<T,N>> {
	template<typename Buffer>
	static bool serialize(Buffer& buffer, std::array<T,N> const& v) {
		return pserialize_range<Backend>(buffer, std::begin(v), std::end(v));
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, std::array<T,N>& v) {
		return pdeserialize_range<Backend>(buffer, std::begin(v), std::end(v));
	}
};
template<template<class> typename Backend, typename T>
struct _PrefixSerializer<Backend, std::vector<T>> {
	template<typename Buffer>
	static bool serialize(Buffer& buffer, std::vector<T> const& v) {
		uint64_t size = v.size();
		if(!::pserialize<Backend>(buffer, size)) {
			return false;
		}
		return pserialize_range<Backend>(buffer, std::begin(v), std::end(v));
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, std::vector<T>& v) {
		uint64_t size;
		if(!::pdeserialize<Backend>(buffer, size)) {
			return false;
		}
		v.resize(size);
		return pdeserialize_range<Backend>(buffer, std::begin(v), std::end(v));
	}
};
template<template<class> typename Backend, typename T>
struct _PrefixSerializer<Backend, std::set<T>> {
	template<typename Buffer>
	static bool serialize(Buffer& buffer, std::set<T> const& v) {
		uint64_t size = v.size();
		if(!::pserialize<Backend>(buffer, size)) {
			return false;
		}
		auto begin = std::begin(v);
		auto end   = std::end(v);
		while(begin != end) {
			if(!::pserialize<Backend>(buffer, *begin)) {
				return false;
			}
			++begin;
		};
		return true;
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, std::set<T>& v) {
		uint64_t size;
		if(!::pdeserialize<Backend>(buffer, size)) {
			return false;
		}
		while(size) {
			--size;
			T vv;
			if(!::pdeserialize<Backend>(buffer, vv)) {
				return false;
			}
			v.insert(vv);
		}
		return true;
	}
};
template<template<class> typename Backend, typename... Ts>
struct _PrefixSerializer<Backend, std::tuple<Ts...>> {
	template<typename Buffer, std::size_t... Is>
	static bool tserialize(Buffer& buffer, std::tuple<Ts...> const& v, std::index_sequence<Is...>) {
		if(!::pserialize<Backend>(buffer, std::get<Is>(v)...)) {
			return false;
		}
		return true;
	}
	template<typename Buffer, std::size_t... Is>
	static bool tdeserialize(Buffer& buffer, std::tuple<Ts...>& v, std::index_sequence<Is...>) {
		auto reset_point = buffer.reset_point();
		if(!::pdeserialize<Backend>(buffer, std::get<Is>(v)...)) {
			return false;
		}
		return true;
	}
	template<typename Buffer>
	static bool serialize(Buffer& buffer, std::tuple<Ts...> const& v) {
		return tserialize(buffer, v, std::make_index_sequence<sizeof...(Ts)>());
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, std::tuple<Ts...>& v) {
		return tdeserialize(buffer, v, std::make_index_sequence<sizeof...(Ts)>());
	}
};
template<template<class> typename Backend, typename T, typename U>
struct _PrefixSerializer<Backend, std::pair<T,U>> {
	template<typename Buffer>
	static bool serialize(Buffer& buffer, std::pair<T,U> const& v) {
		if(!::pserialize<Backend>(buffer, v.first)) {
			return false;
		}
		if(!::pserialize<Backend>(buffer, v.second)) {
			return false;
		}
		return true;
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, std::pair<T,U>& v) {
		if(!::pdeserialize<Backend>(buffer, v.first)) {
			return false;
		}
		if(!::pdeserialize<Backend>(buffer, v.second)) {
			return false;
		}
		return true;
	}
};
template<template<class> typename Backend, typename K, typename V>
struct _PrefixSerializer<Backend, std::map<K,V>> {
	template<typename Buffer>
	static bool serialize(Buffer& buffer, std::map<K,V> const& v) {
		uint64_t size = v.size();
		if(!::pserialize<Backend>(buffer, size)) {
			return false;
		}
		auto begin = std::begin(v);
		auto end   = std::end(v);
		while(begin != end) {
			if(!::pserialize<Backend>(buffer, *begin)) {
				return false;
			}
			++begin;
		};
		return true;
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, std::map<K,V>& v) {
		uint64_t size;
		if(!::pdeserialize<Backend>(buffer, size)) {
			return false;
		}
		while(size) {
			--size;
			std::pair<K,V> vv;
			if(!::pdeserialize<Backend>(buffer, vv)) {
				return false;
			}
			v.insert(vv);
		}
		return true;
	}
};
template<template<class> typename Backend>
struct _PrefixSerializer<Backend, std::string> {
	template<typename Buffer>
	static bool serialize(Buffer& buffer, std::string const& v) {
		uint64_t size = v.size();
		if(!::pserialize<Backend>(buffer, size)) {
			return false;
		}
		return pserialize_range<Backend>(buffer, std::begin(v), std::end(v));
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, std::string& v) {
		uint64_t size;
		if(!::pdeserialize<Backend>(buffer, size)) {
			return false;
		}
		v.resize(size);
		return pdeserialize_range<Backend>(buffer, std::begin(v), std::end(v));
	}
};
template<template<class> typename Backend, typename T>
struct _PrefixSerializer<Backend, std::optional<T>> {
	template<typename Buffer>
	static bool serialize(Buffer& buffer, std::optional<T> const& v) {
		bool has_value = v.has_value();
		if(!::pserialize<Backend>(buffer, has_value)) {
			return false;
		}
		if(has_value && !::pserialize<Backend>(buffer, *v)) {
			return false;
		}
		return true;
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, std::optional<T>& v) {
		bool has_value;
		if(!::pdeserialize<Backend>(buffer, has_value)) {
			return false;
		}
		if(!has_value) {
			v = {};
		} else {
			T vv;
			if(!::pdeserialize<Backend>(buffer, vv)) {
				return false;
			}
			v = vv;
		}
		return true;
	}
};

namespace detail::serializer{
template<std::size_t I, std::size_t N>
struct Loop {
	template<template<class> typename Backend, typename Buffer, typename... Ts>
	static bool serialize(Buffer& buffer, std::variant<Ts...> const& v) {
		using T = std::tuple_element_t<I,std::tuple<Ts...>>;
		if(std::holds_alternative<T>(v)) {
			if(!::pserialize<Backend>(buffer, std::get<T>(v))) {
				return false;
			}
			return true;
		}
		return Loop<I + 1, N>::template serialize<Backend>(buffer, v);
	}
	
	template<template<class> typename Backend, typename Buffer, typename... Ts>
	static bool deserialize(Buffer& buffer, std::variant<Ts...>& v, uint32_t index) {
		if(index == I) {
			using T = std::tuple_element_t<I,std::tuple<Ts...>>;
			T vv;
			if(!::pdeserialize<Backend>(buffer, vv)) {
				return false;
			}
			v = vv;
			return true;
		}
		return Loop<I + 1, N>::template deserialize<Backend>(buffer, v, index);
	}
};

template<std::size_t N>
struct Loop<N,N> {
	template<template<class> typename Backend, typename Buffer, typename... Ts>
	static bool serialize(Buffer&, std::variant<Ts...> const&) {
		return false;
	}
	
	template<template<class> typename Backend, typename Buffer, typename... Ts>
	static bool deserialize(Buffer&, std::variant<Ts...>&, uint32_t) {
		return {};
	}
};
}
template<template<class> typename Backend, typename... Ts>
struct _PrefixSerializer<Backend, std::variant<Ts...>> {
	template<typename Buffer>
	static bool serialize(Buffer& buffer, std::variant<Ts...> const& v) {
		uint32_t index = v.index();
		if(!::pserialize<Backend>(buffer, index)) {
			return false;
		}
		return detail::serializer::Loop<0,sizeof...(Ts)>::template serialize<Backend>(buffer,v);
	}
	template<typename Buffer>
	static bool deserialize(Buffer& buffer, std::variant<Ts...>& v) {
		uint32_t index;
		if(!::pdeserialize<Backend>(buffer, index)) {
			return false;
		}
		return detail::serializer::Loop<0,sizeof...(Ts)>::template deserialize<Backend>(buffer,v, index);
	}
};
