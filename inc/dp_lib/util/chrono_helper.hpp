#pragma once
#include "dp_lib/util/raise.hpp"

#include <chrono>
#include <type_traits>

namespace dp { namespace chrono {

    template<typename Duration, typename Rep, typename Period>
    auto throwing_duration_cast(std::chrono::duration<Rep, Period> d) ->
      typename std::enable_if<!std::is_same<Duration, decltype(d)>::value, Duration>::type {
        using S       = std::chrono::duration<double, typename Duration::period>;
        constexpr S m = Duration::min();
        constexpr S M = Duration::max();
        S const     s = d;
        if(s < m || s > M) { DPRAISE(std::range_error, "throwing_duration_cast failed"); }
        return std::chrono::duration_cast<Duration>(s);
    }

    template<typename Duration>
    constexpr Duration throwing_duration_cast(Duration d) {
        return d;
    }

    template<typename Duration, typename Rep, typename Period>
    auto saturating_duration_cast(std::chrono::duration<Rep, Period> d) ->
      typename std::enable_if<!std::is_same<Duration, decltype(d)>::value, Duration>::type {
        using S       = std::chrono::duration<double, typename Duration::period>;
        constexpr S m = Duration::min();
        constexpr S M = Duration::max();
        S const     s = d;
        if(s < m) { return Duration::min(); }
        if(s > M) { return Duration::max(); }
        return std::chrono::duration_cast<Duration>(s);
    }

    template<typename Duration>
    Duration saturating_duration_cast(Duration d) {
        return d;
    }
}}   // namespace dp::chrono
