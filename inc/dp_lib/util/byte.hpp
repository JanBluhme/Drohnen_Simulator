#pragma once
#include <array>
#include <cstddef>
#include <type_traits>
#include <fmt/format.h>

template<typename STREAM>
STREAM& operator<<(STREAM& o, std::byte b) {
    static std::array<char, 16> const hex_chars{
      {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'}};
    auto const                inx    = std::to_integer<unsigned>(b);
    std::array<char, 5> const buffer = {
      {'0',
       'x',
       hex_chars[(inx >> 4U) & 0x0FU],   //NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
       hex_chars[inx & 0x0FU],           //NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
       '\0'}};
    o << buffer.data();
    return o;
}

template<typename STREAM>
STREAM& operator>>(STREAM& i, std::byte& b) {
    typename std::underlying_type<std::byte>::type tmp;
    i >> tmp;
    b = std::byte(tmp);
    return i;
}

namespace fmt {
template<>
struct formatter<std::byte> : fmt::formatter<unsigned int> {
    template<typename FormatContext>
    auto format(std::byte b, FormatContext& ctx)
      -> decltype(formatter<unsigned int>::format(std::to_integer<unsigned int>(b), ctx)) {
        return formatter<unsigned int>::format(std::to_integer<unsigned int>(b), ctx);
    }
};
}   // namespace fmt
