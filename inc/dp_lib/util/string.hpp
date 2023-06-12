#pragma once

#include <string>

namespace dp { namespace string {
    template<typename T>
    std::string to_string(T&& val) {
        return std::to_string(std::forward<T>(val));
    }

    inline std::string to_string(uint8_t val) { return std::to_string(static_cast<uint32_t>(val)); }

    inline std::string to_string(uint16_t val) { return std::to_string(static_cast<uint32_t>(val)); }

    inline std::string quoted(std::string const& s) { return "\"" + s + "\""; }
}}   // namespace dp::string
