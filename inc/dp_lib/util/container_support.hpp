#pragma once
#include "dp_lib/util/meta_helper.hpp"
#include "dp_lib/util/raise.hpp"

#include <exception>
#include <iterator>

namespace dp {
template<typename It, typename T>
It next(It it, T count) {
    return std::next(it, static_cast<typename std::make_signed<T>::type>(count));
}

namespace detail {
    template<typename T>
    struct output_iterator_traits : std::iterator_traits<T> {};

   /* template<typename OutputIt, typename T>
    struct output_iterator_traits<std::raw_storage_iterator<OutputIt, T>>
      : std::iterator<std::output_iterator_tag, T> {};*/

    template<typename Container>
    struct output_iterator_traits<std::back_insert_iterator<Container>>
      : std::iterator<std::output_iterator_tag, typename Container::value_type> {};

    template<typename Container>
    struct output_iterator_traits<std::front_insert_iterator<Container>>
      : std::iterator<std::output_iterator_tag, typename Container::value_type> {};

    template<typename Container>
    struct output_iterator_traits<std::insert_iterator<Container>>
      : std::iterator<std::output_iterator_tag, typename Container::value_type> {};

    template<typename T, typename charT, typename traits>
    struct output_iterator_traits<std::ostream_iterator<T, charT, traits>>
      : std::iterator<std::output_iterator_tag, T> {};

    template<typename charT, typename traits>
    struct output_iterator_traits<std::ostreambuf_iterator<charT, traits>>
      : std::iterator<std::output_iterator_tag, charT> {};

    struct resize_method_detector {
        template<typename T, typename... Args>
        constexpr auto call(Args... args) const -> decltype(std::declval<T>().resize(args...)) {
            return decltype(std::declval<T>().resize(args...))();
        }
    };

    struct size_method_detector {
        template<typename T, typename... Args>
        constexpr auto call(Args... args) const -> decltype(std::declval<T>().size(args...)) {
            return decltype(std::declval<T>().size(args...))();
        }
    };

    template<typename C>
    constexpr auto container_size(C const& c) -> decltype(c.size()) {
        return c.size();
    }

    template<typename T, std::size_t N>
    //NOLINTNEXTLINE(hicpp-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    constexpr std::size_t container_size(T const (&/*array*/)[N]) noexcept {
        return N;
    }

    template<typename C>
    auto container_resize(std::size_t number_of_elements, C& c) ->
      typename std::enable_if<has_method<C, resize_method_detector, void(std::size_t)>::value>::type {
        c.resize(number_of_elements);
    }

    template<typename C>
    auto container_resize(std::size_t number_of_elements, C& c) ->
      typename std::enable_if<!has_method<C, resize_method_detector, void(std::size_t)>::value>::type {
        // TODO != or > ?????
        if(number_of_elements != container_size(c)) { DPRAISE(std::logic_error, "bad container size"); }
    }

}   // namespace detail
}   // namespace dp
