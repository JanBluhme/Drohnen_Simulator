#pragma once

#include <type_traits>

namespace dp { namespace color {

    enum class style {
        reset       = 0,
        bold        = 1,
        dim         = 2,
        italic      = 3,
        underline   = 4,
        blink       = 5,
        rapid_blink = 6,
        reversed    = 7,
        conceal     = 8,
        crossed     = 9
    };

    enum class foreground {
        black          = 30,
        red            = 31,
        green          = 32,
        yellow         = 33,
        blue           = 34,
        magenta        = 35,
        cyan           = 36,
        white          = 37,
        reset          = 39,
        bright_black   = 90,
        bright_red     = 91,
        bright_green   = 92,
        bright_yellow  = 93,
        bright_blue    = 94,
        bright_magenta = 95,
        bright_cyan    = 96,
        bright_white   = 97
    };

    enum class background {
        black          = 40,
        red            = 41,
        green          = 42,
        yellow         = 43,
        blue           = 44,
        magenta        = 45,
        cyan           = 46,
        white          = 47,
        reset          = 49,
        bright_black   = 100,
        bright_red     = 101,
        bright_green   = 102,
        bright_yellow  = 103,
        bright_blue    = 104,
        bright_magenta = 105,
        bright_cyan    = 106,
        bright_white   = 107
    };

    namespace detail {

        template<typename T, typename STREAM>
        using enableColor =
          typename std::enable_if<std::is_same<T, color::style>::value || std::is_same<T, color::foreground>::value
                                    || std::is_same<T, color::background>::value,
                                  STREAM&>::type;
    }   // namespace detail

    template<typename T, typename STREAM>
    inline detail::enableColor<T, STREAM> operator<<(STREAM& o, T value) {
        o << "\033[" << static_cast<typename std::underlying_type<T>::type>(value) << "m";
        return o;
    }
}}   // namespace dp::color
