#pragma once

#include "dp_lib/util/chrono_helper.hpp"

#include <chrono>

namespace dp {

class FileDescriptorView;

class FileDescriptor {
protected:
    int fd_{-1};

public:
    FileDescriptor() = default;
    explicit FileDescriptor(int fd) noexcept;
    FileDescriptor(FileDescriptor const& other) = delete;
    FileDescriptor& operator=(FileDescriptor const& other) = delete;
    FileDescriptor(FileDescriptor&& other) noexcept;
    FileDescriptor& operator=(FileDescriptor&& other) noexcept;
    ~FileDescriptor() noexcept;

    FileDescriptorView get_view() noexcept;

    void reassign(int new_fd) noexcept;
    int  release() noexcept;
    bool is_valid() const noexcept;

    template<typename Rep, typename Period>
    bool can_recv(std::chrono::duration<Rep, Period> const& timeout) const {
        return poll(timeout, true, false, false, false);
    }

    template<typename Rep, typename Period>
    bool can_send(std::chrono::duration<Rep, Period> const& timeout) const {
        return poll(timeout, false, true, false, false);
    }

    template<typename Rep, typename Period>
    bool poll(const std::chrono::duration<Rep, Period>& timeout,
              bool                                      poll_in,
              bool                                      poll_out,
              bool                                      poll_pri,
              bool                                      poll_error) const {
        return poll_(dp::chrono::saturating_duration_cast<std::chrono::nanoseconds>(timeout),
                     poll_in,
                     poll_out,
                     poll_pri,
                     poll_error);
    }

    int  fd() const noexcept;
    void close() noexcept;

private:
    bool poll_(std::chrono::nanoseconds timeout, bool poll_in, bool poll_out, bool poll_pri, bool poll_error) const;
};

class FileDescriptorView : private FileDescriptor {
public:
    using FileDescriptor::can_recv;
    using FileDescriptor::can_send;
    using FileDescriptor::fd;
    using FileDescriptor::get_view;
    using FileDescriptor::is_valid;
    using FileDescriptor::poll;
    using FileDescriptor::reassign;

    FileDescriptorView() = default;
    explicit FileDescriptorView(int fd) noexcept;
    FileDescriptorView(FileDescriptorView const& other) noexcept;
    FileDescriptorView& operator=(FileDescriptorView const& other) noexcept;
    FileDescriptorView(FileDescriptorView&& other) noexcept;
    FileDescriptorView& operator=(FileDescriptorView&& other) noexcept;
    ~FileDescriptorView() noexcept;
};

}   // namespace dp
