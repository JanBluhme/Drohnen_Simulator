namespace dp {

template<typename T, T I, T... Is, typename O>
void iterate_t(O o) {
    for(T i{}; i < I; ++i) {
        if constexpr(sizeof...(Is) != 0) {
            iterate_t<T, Is...>([&](auto... js) { o(i, js...); });
        } else {
            o(i);
        }
    }
}
template<int... Is, typename O>
void iterate(O o) {
    iterate_t<int, Is...>(o);
}
}   // namespace dp
