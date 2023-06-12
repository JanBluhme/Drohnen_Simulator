#pragma once
#include <concepts>
#include <iterator>

template<typename Iterator, typename T>
concept iterator_of = std::same_as<
	  typename std::iterator_traits<Iterator>::value_type
	, T
>;
