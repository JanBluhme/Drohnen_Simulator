#pragma once

#include "dp_lib/util/container_support.hpp"
#include "dp_lib/util/meta_helper.hpp"
#include "dp_lib/util/raise.hpp"

#include <climits>
#include <iterator>
#include <memory>
#include <numeric>
#include <type_traits>
#include <cstring>

namespace dp {
struct MSB_FIRST {};
struct LSB_FIRST {};

// needs to be forward declared for packing of floats
template<typename C>
struct container_unpacker;
template<typename C>
struct container_packer;
template<typename C>
container_unpacker<C> make_container_unpacker(std::size_t number_of_elements, C& c);
template<typename C>
container_packer<C> make_container_packer(C const& c);

template<typename ByteOrder = MSB_FIRST, typename InputIt, typename... Ts>
InputIt unpack(InputIt first, InputIt last, Ts&&... values);

template<typename ByteOrder = MSB_FIRST, typename InputIt, typename... Ts>
InputIt unpack(InputIt first, Ts&&... values);

namespace detail {

    template<typename ByteOrder, typename OutputIt>
    void serialize(OutputIt /*first*/, OutputIt /*last*/) {}

    template<typename ByteOrder, typename OutputIt>
    void serialize(OutputIt /*first*/) {}

    template<typename ByteOrder, typename InputIt>
    void deserialize(InputIt /*first*/, InputIt /*last*/) {}

    template<typename ByteOrder, typename InputIt>
    void deserialize(InputIt /*first*/) {}

    template<typename ByteOrder, typename OutputIt, typename T>
    struct is_serializable_multi {
        template<typename U>
        //NOLINTNEXTLINE(cert-dcl50-cpp)
        static constexpr std::false_type test(...) noexcept {
            return {};
        }
        template<typename U>
        static constexpr auto test(U const* u) noexcept -> typename std::is_same<
          OutputIt,
          decltype(serialize<ByteOrder, OutputIt>(std::declval<OutputIt>(), std::declval<OutputIt>(), *u))>::type {
            return {};
        }
        //NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        static constexpr bool value = test<T>(nullptr);
    };

    template<typename ByteOrder, typename OutputIt, typename T>
    struct is_serializable {
        template<typename U>
        //NOLINTNEXTLINE(cert-dcl50-cpp)
        static constexpr std::false_type test(...) noexcept {
            return {};
        }
        template<typename U>
        static constexpr auto test(U const* u) noexcept ->
          typename std::is_same<OutputIt,
                                decltype(serialize<ByteOrder, OutputIt>(std::declval<OutputIt>(), *u))>::type {
            return {};
        }
        //NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        static constexpr bool value = test<T>(nullptr);
    };

    template<typename ByteOrder, typename InputIt, typename T>
    struct is_deserializable_multi {
        using TT = typename std::remove_reference<T>::type;
        template<typename U>
        //NOLINTNEXTLINE(cert-dcl50-cpp)
        static constexpr std::false_type test(...) noexcept {
            return {};
        }
        template<typename U>
        static constexpr auto test(U* u) noexcept -> typename std::is_same<
          InputIt,
          decltype(deserialize<ByteOrder, InputIt>(std::declval<InputIt>(), std::declval<InputIt>(), *u))>::type {
            return {};
        }
        //NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        static constexpr bool value = test<TT>(nullptr);
    };

    template<typename ByteOrder, typename InputIt, typename T>
    struct is_deserializable {
        using TT = typename std::remove_reference<T>::type;
        template<typename U>
        //NOLINTNEXTLINE(cert-dcl50-cpp)
        static constexpr std::false_type test(...) noexcept {
            return {};
        }
        template<typename U>
        static constexpr auto test(U* u) noexcept ->
          typename std::is_same<InputIt, decltype(deserialize<ByteOrder, InputIt>(std::declval<InputIt>(), *u))>::type {
            return {};
        }
        //NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        static constexpr bool value = test<TT>(nullptr);
    };

    template<typename T>
    struct custom_make_unsigned {
        using type = typename std::make_unsigned<T>::type;
    };

    template<typename T>
    struct is_integral_helper128 : public std::false_type {};

#ifdef __SIZEOF_INT128__
    template<>
    struct is_integral_helper128<__uint128_t> : public std::true_type {};

    template<>
    struct is_integral_helper128<__int128_t> : public std::true_type {};

    template<>
    struct custom_make_unsigned<__uint128_t> {
        using type = __uint128_t;
    };

    template<>
    struct custom_make_unsigned<__int128_t> {
        using type = __uint128_t;
    };

#endif

    template<typename T>
    struct is_integral_128 : public is_integral_helper128<typename std::remove_cv<T>::type>::type {};

    template<typename T>
    struct is_trivial_serializable {
        using TT                    = typename std::remove_reference<T>::type;
        static constexpr bool value = std::is_integral<TT>::value || dp::detail::is_integral_128<TT>::value
                                      || std::is_enum<TT>::value || std::is_floating_point<TT>::value;
    };

    template<typename T>
    struct has_min_packed_size {
        template<typename... U>
        //NOLINTNEXTLINE(cert-dcl50-cpp)
        static constexpr std::false_type test(...) noexcept {
            return {};
        }
        template<typename U>
        static constexpr auto test(U* u) noexcept ->
          typename std::is_same<std::size_t, decltype(min_packed_size(*u))>::type {
            return {};
        }
        //NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        static constexpr bool value = test<T>(nullptr);
    };

    template<typename T>
    struct has_packed_size {
        template<typename U>
        //NOLINTNEXTLINE(cert-dcl50-cpp)
        static constexpr std::false_type test(...) noexcept {
            return {};
        }
        template<typename U>
        static constexpr auto test(U* u) noexcept ->
          typename std::is_same<std::size_t, decltype(packed_size(*u))>::type {
            return {};
        }
        //NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        static constexpr bool value = test<T>(nullptr);
    };

    template<typename ByteOrder, typename OutputIt, typename T, typename Enable = void>
    struct Pack {};

    template<typename ByteOrder, typename OutputIt, typename T>
    struct Pack<
      ByteOrder,
      OutputIt,
      T,
      typename std::enable_if<detail::disjunction<is_serializable<ByteOrder, OutputIt, T>,
                                                  is_serializable_multi<ByteOrder, OutputIt, T>>::value>::type> {
        static OutputIt impl(OutputIt first, T const& value) { return serialize<ByteOrder, OutputIt>(first, value); }

        static OutputIt impl_multi(OutputIt first, OutputIt last, T const& value) {
            return serialize<ByteOrder, OutputIt>(first, last, value);
        }
    };

    template<typename ByteOrder, typename OutputIt, typename T>
    struct Pack<ByteOrder, OutputIt, T, typename std::enable_if<std::is_enum<T>::value>::type> {
        using underlying_t = typename std::underlying_type<T>::type;

        static OutputIt impl(OutputIt first, T const& value) {
            auto const val = static_cast<underlying_t>(value);
            return Pack<ByteOrder, OutputIt, underlying_t>::impl(first, val);
        }

        static OutputIt impl_multi(OutputIt first, OutputIt last, T const& value) {
            auto const val = static_cast<underlying_t>(value);
            return Pack<ByteOrder, OutputIt, underlying_t>::impl_multi(first, last, val);
        }
    };

    template<typename ByteOrder, typename OutputIt, typename T>
    struct Pack<ByteOrder, OutputIt, T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
        static OutputIt impl(OutputIt first, T const& value) {
            std::array<std::byte, sizeof(T)> buffer{};
            std::memcpy(buffer.data(), std::addressof(value), sizeof(T));
            // TODO reverse dependant on ByteOrder and endianes
            auto const p = dp::make_container_packer(buffer);
            return Pack<ByteOrder, OutputIt, decltype(p)>::impl(first, p);
        }

        static OutputIt impl_multi(OutputIt first, OutputIt last, T const& value) {
            std::array<std::byte, sizeof(T)> buffer{};
            std::memcpy(buffer.data(), std::addressof(value), sizeof(T));
            // TODO reverse dependant on ByteOrder and endianes
            auto const p = dp::make_container_packer(buffer);
            return Pack<ByteOrder, OutputIt, decltype(p)>::impl_multi(first, last, dp::make_container_packer(buffer));
        }
    };

    template<typename OutputIt, typename T>
    struct Pack<LSB_FIRST,
                OutputIt,
                T,
                typename std::enable_if<std::is_integral<T>::value || dp::detail::is_integral_128<T>::value>::type> {
        using unsigned_t = typename custom_make_unsigned<T>::type;
        using value_t    = typename output_iterator_traits<OutputIt>::value_type;

        static OutputIt impl(OutputIt first, T const& value) {
            for(std::size_t i{}; i < sizeof(T); ++i) {
                *first = static_cast<value_t>(static_cast<unsigned_t>(value) >> (CHAR_BIT * i));
                ++first;
            }
            return first;
        }

        static OutputIt impl_multi(OutputIt first, OutputIt last, T const& value) {
            auto const dist = std::distance(first, last);

            if(dist < 0) { DPRAISE(std::out_of_range, "fist and last do not build a valid range"); }

            if(static_cast<typename std::make_unsigned<decltype(dist)>::type>(dist) < sizeof(T)) {
                DPRAISE(std::out_of_range, "output range to small to pack elements");
            }

            return impl(first, value);
        }
    };

    template<typename OutputIt, typename T>
    struct Pack<MSB_FIRST,
                OutputIt,
                T,
                typename std::enable_if<std::is_integral<T>::value || dp::detail::is_integral_128<T>::value>::type> {
        using unsigned_t = typename custom_make_unsigned<T>::type;
        using value_t    = typename output_iterator_traits<OutputIt>::value_type;

        static OutputIt impl(OutputIt first, T const& value) {
            for(std::size_t i = sizeof(T); i != 0; --i) {
                auto const tmp = static_cast<value_t>(static_cast<unsigned_t>(value) >> (CHAR_BIT * (i - 1)));
                *first         = tmp;
                ++first;
            }
            return first;
        }

        static OutputIt impl_multi(OutputIt first, OutputIt last, T const& value) {
            auto const dist = std::distance(first, last);

            if(dist < 0) { DPRAISE(std::out_of_range, "fist and last do not build a valid range"); }

            if(static_cast<typename std::make_unsigned<decltype(dist)>::type>(dist) < sizeof(T)) {
                DPRAISE(std::out_of_range, "output range to small to pack elements");
            }

            return impl(first, value);
        }
    };

    template<typename ByteOrder, typename InputIt, typename T, typename Enable = void>
    struct Unpack {};

    template<typename ByteOrder, typename InputIt, typename T>
    struct Unpack<
      ByteOrder,
      InputIt,
      T,
      typename std::enable_if<detail::disjunction<
        is_deserializable<ByteOrder, InputIt, typename std::remove_reference<T>::type>,
        is_deserializable_multi<ByteOrder, InputIt, typename std::remove_reference<T>::type>>::value>::type> {
        static InputIt impl(InputIt first, T&& value) {
            return deserialize<ByteOrder, InputIt>(first, std::forward<T>(value));
        }

        static InputIt impl_multi(InputIt first, InputIt last, T&& value) {
            return deserialize<ByteOrder, InputIt>(first, last, std::forward<T>(value));
        }
    };

    template<typename ByteOrder, typename InputIt, typename T>
    struct Unpack<ByteOrder,
                  InputIt,
                  T,
                  typename std::enable_if<std::is_enum<typename std::remove_reference<T>::type>::value>::type> {
        using TT           = typename std::remove_reference<T>::type;
        using underlying_t = typename std::underlying_type<TT>::type;

        static InputIt impl(InputIt first, T&& value) {
            underlying_t val;
            auto const   ret = Unpack<ByteOrder, InputIt, underlying_t>::impl(first, val);
            value            = static_cast<TT>(val);
            return ret;
        }

        static InputIt impl_multi(InputIt first, InputIt last, T&& value) {
            underlying_t val;
            auto         ret = Unpack<ByteOrder, InputIt, underlying_t>::impl_multi(first, last, val);
            value            = static_cast<TT>(val);
            return ret;
        }
    };

    template<typename ByteOrder, typename InputIt, typename T>
    struct Unpack<
      ByteOrder,
      InputIt,
      T,
      typename std::enable_if<std::is_floating_point<typename std::remove_reference<T>::type>::value>::type> {
        using TT = typename std::remove_reference<T>::type;
        static InputIt impl(InputIt first, T&& value) {
            std::array<std::uint8_t, sizeof(T)> buffer{};
            auto const ret = dp::unpack<ByteOrder>(first, dp::make_container_unpacker(sizeof(T), buffer));
            std::memcpy(std::addressof(value), buffer.data(), sizeof(TT));
            return ret;
        }

        static InputIt impl_multi(InputIt first, InputIt last, T& value) {
            std::array<std::uint8_t, sizeof(T)> buffer{};
            auto const ret = dp::unpack<ByteOrder>(first, last, dp::make_container_unpacker(sizeof(T), buffer));
            std::memcpy(std::addressof(value), buffer.data(), sizeof(TT));
            return ret;
        }
    };

    template<typename InputIt, typename T>
    struct Unpack<
      LSB_FIRST,
      InputIt,
      T,
      typename std::enable_if<std::is_integral<typename std::remove_reference<T>::type>::value
                              || dp::detail::is_integral_128<typename std::remove_reference<T>::type>::value>::type> {
        using TT         = typename std::remove_reference<T>::type;
        using unsigned_t = typename custom_make_unsigned<TT>::type;

        static InputIt impl(InputIt first, T& value) {
            unsigned_t tmp{};
            for(std::size_t i{}; i < sizeof(TT); ++i) {
                tmp |= static_cast<unsigned_t>(static_cast<std::uint8_t>(*first)) << (CHAR_BIT * i);
                ++first;
            }

            value = static_cast<TT>(tmp);
            return first;
        }

        static InputIt impl_multi(InputIt first, InputIt last, T& value) {
            auto const dist = std::distance(first, last);

            if(dist < 0) { DPRAISE(std::out_of_range, "fist and last do not build a valid range"); }

            if(static_cast<typename std::make_unsigned<decltype(dist)>::type>(dist) < sizeof(TT)) {
                DPRAISE(std::out_of_range, "input range to small to unpack element(s)");
            }

            return impl(first, value);
        }
    };

    template<typename InputIt, typename T>
    struct Unpack<
      MSB_FIRST,
      InputIt,
      T,
      typename std::enable_if<std::is_integral<typename std::remove_reference<T>::type>::value
                              || dp::detail::is_integral_128<typename std::remove_reference<T>::type>::value>::type> {
        using TT         = typename std::remove_reference<T>::type;
        using unsigned_t = typename custom_make_unsigned<TT>::type;

        static InputIt impl(InputIt first, T& value) {
            unsigned_t tmp{};
            for(std::size_t i = sizeof(TT); i != 0; --i) {
                tmp |= static_cast<unsigned_t>(static_cast<unsigned_t>(static_cast<std::uint8_t>(*first))
                                               << static_cast<unsigned_t>((CHAR_BIT * (i - 1))));
                ++first;
            }

            value = static_cast<TT>(tmp);
            return first;
        }

        static InputIt impl_multi(InputIt first, InputIt last, T& value) {
            auto const dist = std::distance(first, last);

            if(dist < 0) { DPRAISE(std::out_of_range, "fist and last do not build a valid range"); }

            if(static_cast<typename std::make_unsigned<decltype(dist)>::type>(dist) < sizeof(TT)) {
                DPRAISE(std::out_of_range, "input range to small to unpack element(s)");
            }

            return impl(first, value);
        }
    };

    template<typename ByteOrder, typename OutputIt, typename... Ts>
    OutputIt pack(OutputIt first, Ts const&... /*values*/) {
        return first;
    }

    template<typename ByteOrder, typename OutputIt, typename T, typename... Ts>
    OutputIt pack(OutputIt first, T const& value, Ts const&... values) {
        auto const next = detail::Pack<ByteOrder, OutputIt, T>::impl(first, value);
        return detail::pack<ByteOrder>(next, values...);
    }

    template<typename ByteOrder, typename OutputIt, typename... Ts>
    OutputIt pack(OutputIt first, OutputIt /*last*/, Ts const&... /*values*/) {
        return first;
    }

    template<typename ByteOrder, typename OutputIt, typename T, typename... Ts>
    OutputIt pack(OutputIt first, OutputIt last, T const& value, Ts const&... values) {
        auto const next = detail::Pack<ByteOrder, OutputIt, T>::impl_multi(first, last, value);
        return detail::pack<ByteOrder>(next, last, values...);
    }

    template<typename ByteOrder, typename InputIt, typename... Ts>
    InputIt unpack(InputIt first, Ts&&... /*values*/) {
        return first;
    }

    template<typename ByteOrder, typename InputIt, typename T, typename... Ts>
    InputIt unpack(InputIt first, T&& value, Ts&&... values) {
        auto const next = detail::Unpack<ByteOrder, InputIt, T>::impl(first, std::forward<T>(value));
        return detail::unpack<ByteOrder>(next, std::forward<Ts>(values)...);
    }

    template<typename ByteOrder, typename InputIt, typename... Ts>
    InputIt unpack(InputIt first, InputIt /*last*/, Ts&&... /*values*/) {
        return first;
    }

    template<typename ByteOrder, typename InputIt, typename T, typename... Ts>
    InputIt unpack(InputIt first, InputIt last, T&& value, Ts&&... values) {
        auto const next = detail::Unpack<ByteOrder, InputIt, T>::impl_multi(first, last, std::forward<T>(value));
        return detail::unpack<ByteOrder>(next, last, std::forward<Ts>(values)...);
    }

    template<typename T, typename Enable = void>
    struct Get_min_packed_size {};

    template<typename T>
    struct Get_min_packed_size<T, typename std::enable_if<has_min_packed_size<T>::value>::type> {
        static constexpr std::size_t impl(T const& t) { return min_packed_size(t); }
    };

    template<typename T>
    struct Get_min_packed_size<T, typename std::enable_if<is_trivial_serializable<T>::value>::type> {
        static constexpr std::size_t impl(T const& t) { return sizeof(t); }
    };

    template<typename... Ts>
    constexpr std::size_t get_min_packed_size(Ts const&... /*ts*/) {
        return 0;
    }

    template<typename T, typename... Ts>
    constexpr std::size_t get_min_packed_size(T const& t, Ts const&... ts) {
        return Get_min_packed_size<T>::impl(t) + get_min_packed_size(ts...);
    }

    template<typename T, typename Enable = void>
    struct Get_packed_size {};

    template<typename T>
    struct Get_packed_size<T, typename std::enable_if<has_packed_size<T>::value>::type> {
        static constexpr std::size_t impl(T const& t) { return packed_size(t); }
    };

    template<typename T>
    struct Get_packed_size<T, typename std::enable_if<is_trivial_serializable<T>::value>::type> {
        static constexpr std::size_t impl(T const& t) { return sizeof(t); }
    };

    template<typename... Ts>
    constexpr std::size_t get_packed_size(Ts const&... /*ts*/) {
        return 0;
    }

    template<typename T, typename... Ts>
    constexpr std::size_t get_packed_size(T const& t, Ts const&... ts) {
        return Get_packed_size<T>::impl(t) + get_packed_size(ts...);
    }
}   // namespace detail

template<typename ByteOrder = MSB_FIRST, typename OutputIt, typename... Ts>
OutputIt pack(OutputIt first, OutputIt last, Ts const&... values) {
    static_assert(
      detail::disjunction<
        std::is_same<typename std::iterator_traits<OutputIt>::iterator_category, std::forward_iterator_tag>,
        std::is_same<typename std::iterator_traits<OutputIt>::iterator_category, std::random_access_iterator_tag>,
        std::is_same<typename std::iterator_traits<OutputIt>::iterator_category,
                     std::bidirectional_iterator_tag>>::value,
      "not an multi pass iterator");
    static_assert(sizeof(typename detail::output_iterator_traits<OutputIt>::value_type) == 1, "only with 8bit types");
    static_assert(detail::all_true<(detail::is_trivial_serializable<Ts>::value
                                    || detail::is_serializable_multi<ByteOrder, OutputIt, Ts>::value)...>::value,
                  "only with (integral_type || enum || floating_point || Serializable_multi)");

    auto const dist = std::distance(first, last);
    if(dist < 0) { DPRAISE(std::out_of_range, "fist and last do not build a valid range"); }

    if(static_cast<typename std::make_unsigned<decltype(dist)>::type>(dist) < detail::get_packed_size(values...)) {
        DPRAISE(std::out_of_range, "output range to small to pack element(s)");
    }

    return detail::pack<ByteOrder, OutputIt>(first, last, values...);
}

template<typename ByteOrder = MSB_FIRST, typename OutputIt, typename... Ts>
OutputIt pack(OutputIt first, Ts const&... values) {
    static_assert(
      detail::disjunction<
        std::is_same<typename std::iterator_traits<OutputIt>::iterator_category, std::forward_iterator_tag>,
        std::is_same<typename std::iterator_traits<OutputIt>::iterator_category, std::random_access_iterator_tag>,
        std::is_same<typename std::iterator_traits<OutputIt>::iterator_category, std::bidirectional_iterator_tag>,
        std::is_same<typename std::iterator_traits<OutputIt>::iterator_category, std::output_iterator_tag>>::value,
      "not an output iterator");
    static_assert(sizeof(typename detail::output_iterator_traits<OutputIt>::value_type) == 1, "only with 8bit types");
    static_assert(detail::all_true<(detail::is_trivial_serializable<Ts>::value
                                    || detail::is_serializable<ByteOrder, OutputIt, Ts>::value)...>::value,
                  "only with (integral_type || enum || floating_point || Serializable)");

    return detail::pack<ByteOrder, OutputIt>(first, values...);
}

template<typename ByteOrder, typename InputIt, typename... Ts>
InputIt unpack(InputIt first, InputIt last, Ts&&... values) {
    static_assert(
      detail::disjunction<
        std::is_same<typename std::iterator_traits<InputIt>::iterator_category, std::forward_iterator_tag>,
        std::is_same<typename std::iterator_traits<InputIt>::iterator_category, std::random_access_iterator_tag>,
        std::is_same<typename std::iterator_traits<InputIt>::iterator_category,
                     std::bidirectional_iterator_tag>>::value,
      "not an multi pass iterator");
    static_assert(sizeof(typename std::iterator_traits<InputIt>::value_type) == 1, "only with 8bit types");
    static_assert(detail::all_true<(detail::is_trivial_serializable<Ts>::value
                                    || detail::is_deserializable_multi<ByteOrder, InputIt, Ts>::value)...>::value,
                  "only with (integral_type || enum || floating_point || Deserializable_multi)");

    auto const dist = std::distance(first, last);

    if(dist < 0) { DPRAISE(std::out_of_range, "fist and last do not build a valid range"); }

    if(static_cast<typename std::make_unsigned<decltype(dist)>::type>(dist)
       < detail::get_min_packed_size(std::forward<Ts>(values)...)) {
        DPRAISE(std::out_of_range, "input range to small to unpack element(s)");
    }

    return detail::unpack<ByteOrder, InputIt>(first, last, std::forward<Ts>(values)...);
}

template<typename ByteOrder, typename InputIt, typename... Ts>
InputIt unpack(InputIt first, Ts&&... values) {
    static_assert(
      detail::disjunction<
        std::is_same<typename std::iterator_traits<InputIt>::iterator_category, std::forward_iterator_tag>,
        std::is_same<typename std::iterator_traits<InputIt>::iterator_category, std::random_access_iterator_tag>,
        std::is_same<typename std::iterator_traits<InputIt>::iterator_category, std::bidirectional_iterator_tag>,
        std::is_same<typename std::iterator_traits<InputIt>::iterator_category, std::input_iterator_tag>>::value,
      "not an input iterator");
    static_assert(sizeof(typename std::iterator_traits<InputIt>::value_type) == 1, "only with 8bit types");
    static_assert(detail::all_true<(detail::is_trivial_serializable<Ts>::value
                                    || detail::is_deserializable<ByteOrder, InputIt, Ts>::value)...>::value,
                  "only with (integral_type || enum || floating_point || Deserializable)");

    return detail::unpack<ByteOrder, InputIt>(first, std::forward<Ts>(values)...);
}

template<typename C>
struct container_packer {
    explicit container_packer(C const& c_) : c{c_} {}

    C const& c;
};

template<typename ByteOrder, typename OutputIt, typename C>
OutputIt serialize(OutputIt first, OutputIt last, container_packer<C> const& cp) {
    auto next = first;
    for(const auto& e : cp.c) { next = dp::pack<ByteOrder>(next, last, e); }
    return next;
}

template<typename ByteOrder, typename OutputIt, typename C>
OutputIt serialize(OutputIt first, container_packer<C> const& cp) {
    auto next = first;
    for(auto const& e : cp.c) { next = dp::pack<ByteOrder>(next, e); }
    return next;
}

template<typename C>
std::size_t packed_size(container_packer<C> const& cp) {
    using std::begin;
    using std::end;
    return std::accumulate(begin(cp.c),
                           end(cp.c),
                           std::size_t{},
                           [](std::size_t sum, typename std::decay<decltype(*begin(cp.c))>::type const& e) {
                               return sum + dp::detail::get_packed_size(e);
                           });
}

template<typename C>
container_packer<C> make_container_packer(C const& c) {
    return container_packer<C>{c};
}

template<typename C>
struct container_unpacker {
    container_unpacker(std::size_t number_of_elements_, C& c_) : number_of_elements{number_of_elements_}, c{c_} {}
    std::size_t number_of_elements;
    C&          c;
};

template<typename C, typename UnpackType, typename ConvertFunction>
struct converting_container_unpacker {
    converting_container_unpacker(std::size_t number_of_elements_, C& c_, ConvertFunction f_)
      : number_of_elements{number_of_elements_}
      , c{c_}
      , f{f_} {}
    std::size_t     number_of_elements;
    C&              c;
    ConvertFunction f;
};

template<typename ByteOrder, typename InputIt, typename C>
InputIt deserialize(InputIt first, InputIt last, container_unpacker<C> cp) {
    detail::container_resize(cp.number_of_elements, cp.c);
    auto next = first;
    for(auto& e : cp.c) { next = dp::unpack<ByteOrder>(next, last, e); }
    return next;
}

template<typename ByteOrder, typename InputIt, typename C>
InputIt deserialize(InputIt first, container_unpacker<C> cp) {
    detail::container_resize(cp.number_of_elements, cp.c);
    auto next = first;
    for(auto& e : cp.c) { next = dp::unpack<ByteOrder>(next, e); }
    return next;
}

template<typename C>
constexpr std::size_t min_packed_size(container_unpacker<C> const& /*cp*/) {
    return 0;
}

template<typename ByteOrder, typename InputIt, typename C, typename UnpackType, typename ConvertFunction>
InputIt deserialize(InputIt first, InputIt last, converting_container_unpacker<C, UnpackType, ConvertFunction> cp) {
    detail::container_resize(cp.number_of_elements, cp.c);
    auto next = first;
    for(auto& e : cp.c) {
        UnpackType v;
        next = dp::unpack<ByteOrder>(next, last, v);
        e    = cp.f(v);
    }
    return next;
}

template<typename ByteOrder, typename InputIt, typename C, typename UnpackType, typename ConvertFunction>
InputIt deserialize(InputIt first, converting_container_unpacker<C, UnpackType, ConvertFunction> cp) {
    detail::container_resize(cp.number_of_elements, cp.c);
    auto next = first;
    for(auto& e : cp.c) {
        UnpackType v;
        next = dp::unpack<ByteOrder>(next, v);
        e    = cp.f(v);
    }
    return next;
}

template<typename C, typename UnpackType, typename ConvertFunction>
constexpr std::size_t min_packed_size(converting_container_unpacker<C, UnpackType, ConvertFunction> const& /*cp*/) {
    return 0;
}

template<typename C>
container_unpacker<C> make_container_unpacker(std::size_t number_of_elements, C& c) {
    return container_unpacker<C>{number_of_elements, c};
}

template<typename UnpackType, typename C, typename ConvertFunction>
converting_container_unpacker<C, UnpackType, ConvertFunction>
  make_converting_container_unpacker(std::size_t number_of_elements, C& c, ConvertFunction f) {
    return converting_container_unpacker<C, UnpackType, ConvertFunction>{number_of_elements, c, f};
}
}   // namespace dp
