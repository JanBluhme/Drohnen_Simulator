#pragma once

#include <utility>

namespace dp {
namespace detail {

    template<typename, typename, typename T>
    struct has_method {
        static_assert(std::integral_constant<T, false>::value,
                      "Third template parameter needs to be of function type.");
    };

    template<typename C, typename caller, typename Ret, typename... Args>
    struct has_method<C, caller, Ret(Args...)> {
        template<typename T>
        static constexpr auto check(T * /*unused*/) ->
          typename std::is_same<decltype(std::declval<caller>().template call<T>(std::declval<Args>()...)), Ret>::type {
            return typename std::is_same<decltype(std::declval<caller>().template call<T>(std::declval<Args>()...)),
                                         Ret>::type();
            // return to surpresswarnings
        }

        template<typename>
        //NOLINTNEXTLINE(cert-dcl50-cpp)
        static constexpr std::false_type check(...) {
            return {};
        }

        //NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
        using type = decltype(check<C>(nullptr));

        static constexpr bool value = type::value;
    };

    template<bool...>
    struct bool_pack;
    template<bool... bs>
    using all_true = std::is_same<bool_pack<bs..., true>, bool_pack<true, bs...>>;

    template<typename...>
    struct disjunction : std::false_type {};
    template<typename B1>
    struct disjunction<B1> : B1 {};
    template<typename B1, typename... Bn>
    struct disjunction<B1, Bn...> : std::conditional<bool(B1::value), B1, disjunction<Bn...>>::type {};

}   // namespace detail

template<typename T>
struct MetaPrint;

}   // namespace dp
