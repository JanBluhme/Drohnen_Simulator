#include <functional>
#include <utility>

namespace dp {

template<template<typename...> typename T, typename... TTs, typename F>
void for_each_in_tuple(T<TTs...> const& t, F f) {
    std::apply([&](auto const&... x) { [[maybe_unused]] int l[]{(f(x), 0)..., 0}; }, t);
}

template<template<typename...> typename T, typename... TTs, typename F>
void for_each_in_tuple(T<TTs...>& t, F f) {
    std::apply([&](auto&... x) { [[maybe_unused]] int l[]{(f(x), 0)..., 0}; }, t);
}

template<typename, typename>
struct TupleCat;

template<template<typename...> typename T, typename... First, typename... Second>
struct TupleCat<T<First...>, T<Second...>> {
    using type = T<First..., Second...>;
};

template<typename T1, typename T2>
using TupleCat_t = typename TupleCat<T1, T2>::type;

}   // namespace dp
