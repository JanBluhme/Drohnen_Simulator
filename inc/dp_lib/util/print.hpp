#pragma once

#include "dp_lib/util/container_support.hpp"
#include "dp_lib/util/log.hpp"

#include <cstddef>
#include <type_traits>
#include <utility>

namespace dp {

template<typename InputIt>
struct Range_Printer {
    Range_Printer(InputIt first_, InputIt last_) : first{first_}, last{last_} {}
    InputIt first;
    InputIt last;
};

template<typename T, typename STREAM>
STREAM& operator<<(STREAM& o, Range_Printer<T> const& printer) {
    auto       it  = printer.first;
    auto const end = printer.last;
    o << '[';
    if(it != end) {
        o << *it;
        ++it;
    }
    while(it != end) {
        o << ", " << *it;
        ++it;
    }
    o << ']';
    return o;
}

template<typename InputIt>
Range_Printer<typename std::add_const<InputIt>::type> make_range_printer(InputIt first, InputIt last) {
    return Range_Printer<typename std::add_const<InputIt>::type>(first, last);
}

template<typename InputIt>
Range_Printer<typename std::add_const<InputIt>::type> make_range_printer(InputIt first, std::size_t size) {
    return Range_Printer<typename std::add_const<InputIt>::type>(first, dp::next(first, size));
}

template<typename C>
auto make_range_printer(C const& container) -> Range_Printer<decltype(begin(container))> {
    using std::begin;
    return Range_Printer<decltype(begin(container))>(begin(container), end(container));
}

}   // namespace dp

#if DPLOG_ACTIVE_LEVEL != DPLOG_LEVEL_OFF
namespace fmt {
template<typename T>
struct formatter<dp::Range_Printer<T>> : fmt::formatter<typename std::decay<decltype(*std::declval<T>())>::type> {
    template<typename FormatContext>
    auto format(dp::Range_Printer<T> const& rp, FormatContext& ctx) -> decltype(fmt::format_to(ctx.out(), "]")) {
        auto       it  = rp.first;
        auto const end = rp.last;
        fmt::format_to(ctx.out(), "[");
        if(it != end) {
            fmt::formatter<typename std::decay<decltype(*std::declval<T>())>::type>::format(*it, ctx);
            ++it;
        }
        while(it != end) {
            fmt::format_to(ctx.out(), ", ");
            fmt::formatter<typename std::decay<decltype(*std::declval<T>())>::type>::format(*it, ctx);
            ++it;
        }
        return fmt::format_to(ctx.out(), "]");
    }
};
}   // namespace fmt
#endif
