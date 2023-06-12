#include "dp_lib/util/file_helper.hpp"

#include "dp_lib/util/FileDescriptor.hpp"
#include "dp_lib/util/container_support.hpp"
#include "dp_lib/util/system_call_helper.hpp"

#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <vector>

namespace dp { namespace file {
    std::string read_to_string(std::string const& filename) {
        //NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg, hicpp-signed-bitwise)
        FileDescriptor fd{::open(filename.c_str(), O_RDONLY | O_CLOEXEC)};
        if(!fd.is_valid()) { DPRAISE_SYSTEM_ERROR("open failed"); }

        std::vector<char> content{};
        std::size_t       bytesRead{};
        while(true) {
            content.resize(content.size() + 1024);
            auto const status = ::read(fd.fd(), dp::next(content.data(), bytesRead), content.size() - bytesRead);
            if(status == -1) {
                if(is_errno_recoverable(errno)) { continue; }
                DPRAISE_SYSTEM_ERROR("read failed");
            }
            if(status == 0) { break; }
            bytesRead += static_cast<std::size_t>(status);
        }

        return std::string{content.begin(), dp::next(content.begin(), bytesRead)};
    }

    void write_to_file(std::string const& filename, std::string const& text) {
        //NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg, hicpp-signed-bitwise)
        FileDescriptor fd{::open(filename.c_str(), O_CREAT | O_WRONLY | O_CLOEXEC, mode_t(0644))};
        if(!fd.is_valid()) { DPRAISE_SYSTEM_ERROR("open failed"); }

        std::size_t bytesWritten{};
        while(true) {
            auto const status = ::write(fd.fd(), dp::next(text.data(), bytesWritten), text.size() - bytesWritten);
            if(status == -1) {
                if(is_errno_recoverable(errno)) { continue; }
                DPRAISE_SYSTEM_ERROR("write failed");
            }
            if(status == 0) { break; }
            bytesWritten += static_cast<std::size_t>(status);
            if(bytesWritten == text.size()) { break; }
        }
    }
}}   // namespace dp::file
