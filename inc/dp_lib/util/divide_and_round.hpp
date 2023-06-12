#pragma once

#include <limits>
#include <type_traits>

namespace dp {
template<typename T>
typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value, T>::type
  divide_and_round_to_nearest(T numerator, T denominator) {
    T reminder = numerator % denominator;
    T quotient = numerator / denominator;

    if(reminder > std::numeric_limits<T>::max() / 2) {
        if((reminder) >= ((denominator / 2))) { quotient += 1; }
    } else {
        if((reminder * 2) >= ((denominator))) { quotient += 1; }
    }

    return quotient;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value, T>::type
  divide_and_round_to_zero(T numerator, T denominator) {
    return numerator / denominator;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value, T>::type
  divide_and_round_up(T numerator, T denominator) {
    auto reminder = numerator % denominator;
    auto quotient = numerator / denominator;

    if(reminder != 0) { quotient += 1; }

    return quotient;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value, T>::type
  divide_and_round_down(T numerator, T denominator) {
    return numerator / denominator;
}
}   // namespace dp
