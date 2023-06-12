#include "dp_lib/util/FileDescriptor.hpp"

#include "dp_lib/util/raise.hpp"

#include <cerrno>
#include <ctime>
#include <limits>
#include <poll.h>
#include <stdexcept>
#include <unistd.h>

namespace dp {

FileDescriptor::FileDescriptor(int fd) noexcept : fd_(fd) {}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept {
    if(this != std::addressof(other)) {
        fd_       = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

FileDescriptor::~FileDescriptor() noexcept {
    close();
}

FileDescriptorView FileDescriptor::get_view() noexcept {
    FileDescriptorView fdv(fd_);
    return fdv;
}

void FileDescriptor::reassign(int new_fd) noexcept {
    close();
    fd_ = new_fd;
}

int FileDescriptor::release() noexcept {
    int fd = fd_;
    fd_    = -1;
    return fd;
}

bool FileDescriptor::is_valid() const noexcept {
    return fd_ != -1;
}

int FileDescriptor::fd() const noexcept {
    return fd_;
}

void FileDescriptor::close() noexcept {
    if(is_valid()) {
        int const status = ::close(fd_);
        fd_              = -1;
        // On linux close will always close even on failure and errno EINTR
        if(-1 == status) {
            try {
                DPRAISE_SYSTEM_ERROR_PRINT_ONLY("close failed");
            } catch(...) {}
        }
    }
}

bool FileDescriptor::poll_(std::chrono::nanoseconds timeout,
                           bool                     poll_in,
                           bool                     poll_out,
                           bool                     poll_pri,
                           bool                     poll_error) const {
    timeout     = std::chrono::nanoseconds{} > timeout ? std::chrono::nanoseconds{} : timeout;
    auto calcTs = [](std::chrono::nanoseconds timeout_) {
        timeout_                       = std::chrono::nanoseconds{} > timeout_ ? std::chrono::nanoseconds{} : timeout_;
        std::chrono::seconds const sec = std::chrono::duration_cast<std::chrono::seconds>(timeout_);
        timespec                   ts{};
        if(sec.count() >= std::numeric_limits<decltype(ts.tv_sec)>::max()) {
            ts.tv_sec  = std::numeric_limits<decltype(ts.tv_sec)>::max();
            ts.tv_nsec = 0;
        } else {
            ts.tv_sec  = static_cast<decltype(ts.tv_sec)>(sec.count());
            ts.tv_nsec = static_cast<decltype(ts.tv_nsec)>(
              std::chrono::duration_cast<std::chrono::nanoseconds>(timeout_ - sec).count());
        }
        return ts;
    };

    pollfd fds{};

    fds.fd     = fd_;
    fds.events = 0;
    if(poll_in) { fds.events |= POLLIN; }         //NOLINT(hicpp-signed-bitwise)
    if(poll_out) { fds.events |= POLLOUT; }       //NOLINT(hicpp-signed-bitwise)
    if(poll_pri) { fds.events |= POLLPRI; }       //NOLINT(hicpp-signed-bitwise)
    if(poll_error) { fds.events |= POLLRDHUP; }   //NOLINT(hicpp-signed-bitwise)

    auto const stoptime = timeout > std::chrono::hours(24 * 365 * 100)
                            ? std::chrono::steady_clock::time_point::max()
                            : std::chrono::steady_clock::now()
                                + timeout;   // could overflow but the program run for ~191 years so that is OK

    auto ts = calcTs(timeout);

    while(true) {
        int const status = ::ppoll(std::addressof(fds), 1, std::addressof(ts), nullptr);
        if(status == -1) {
            if(errno == EINTR) {
                if(stoptime != std::chrono::steady_clock::time_point::max()) {
                    auto const now = std::chrono::steady_clock::now();
                    if(now >= stoptime) { return false; }
                    ts = calcTs(std::chrono::duration_cast<std::chrono::nanoseconds>(stoptime - now));
                }
                continue;
            }
            DPRAISE_SYSTEM_ERROR("ppoll failed");
        } else if(status == 0) {
            auto const now = std::chrono::steady_clock::now();
            if(now < stoptime) {
                ts = calcTs(std::chrono::duration_cast<std::chrono::nanoseconds>(stoptime - now));
                continue;
            }
            return false;
        } else {
            if((fds.revents & POLLNVAL) != 0) {   //NOLINT(hicpp-signed-bitwise)
                DPRAISE(std::runtime_error, "poll_ failed POLLNVAL");
            }
            return ((fds.revents & POLLIN) != 0)                          //NOLINT(hicpp-signed-bitwise)
                   || ((fds.revents & POLLOUT) != 0)                      //NOLINT(hicpp-signed-bitwise)
                   || ((fds.revents & POLLPRI) != 0)                      //NOLINT(hicpp-signed-bitwise)
                   || (poll_error ? (((fds.revents & POLLRDHUP) != 0)     //NOLINT(hicpp-signed-bitwise)
                                     || ((fds.revents & POLLERR) != 0)    //NOLINT(hicpp-signed-bitwise)
                                     || ((fds.revents & POLLHUP) != 0))   //NOLINT(hicpp-signed-bitwise)
                                  : false);
        }
    }
}

FileDescriptorView::FileDescriptorView(int fd) noexcept : FileDescriptor{fd} {}

FileDescriptorView::FileDescriptorView(FileDescriptorView const& other) noexcept : FileDescriptor{other.fd_} {}

FileDescriptorView& FileDescriptorView::operator=(FileDescriptorView const& other) noexcept {
    if(this != std::addressof(other)) { fd_ = other.fd_; }
    return *this;
}

FileDescriptorView::FileDescriptorView(FileDescriptorView&& other) noexcept : FileDescriptor{other.fd_} {
    other.fd_ = -1;
}

FileDescriptorView& FileDescriptorView::operator=(FileDescriptorView&& other) noexcept {
    if(this != std::addressof(other)) {
        fd_       = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

FileDescriptorView::~FileDescriptorView() noexcept {
    fd_ = -1;
}

}   // namespace dp
