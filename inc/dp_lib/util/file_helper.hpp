#pragma once

#include <string>

namespace dp { namespace file {
    std::string read_to_string(std::string const& filename);
    void        write_to_file(std::string const& filename, std::string const& text);
}}   // namespace dp::file
