#pragma once

#include "dp_lib/util/FileDescriptor.hpp"
#include "dp_lib/util/chrono_helper.hpp"
#include "dp_lib/util/priority.hpp"

#include <chrono>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

namespace dp {
class Process {
public:
    template<typename... Args>
    explicit Process(std::string const& application, Args&&... args) : pid_{-1}
                                                                     , exitStatus_{0} {
        //NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay, hicpp-no-array-decay)
        start(application, std::vector<std::string>{std::forward<Args>(args)...}, priority::Inheritance::Reset);
    }

    template<typename... Args>
    Process(priority::Inheritance pi, std::string const& application, Args&&... args) : pid_{-1}
                                                                                      , exitStatus_{0} {
        //NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay, hicpp-no-array-decay)
        start(application, std::vector<std::string>{std::forward<Args>(args)...}, pi);
    }

    Process(Process const& other) = delete;
    Process& operator=(Process const& other) = delete;
    Process(Process&& other) noexcept;
    Process& operator=(Process&& other) noexcept;

    ~Process() noexcept;

    void signal(int signal);

    template<typename Rep, typename Period>
    void wait(std::chrono::duration<Rep, Period> const& timeout) {
        wait_internal(dp::chrono::saturating_duration_cast<std::chrono::nanoseconds>(timeout));
    }

    bool running();

    int exit_status();

    template<typename Rep, typename Period>
    void terminate(std::chrono::duration<Rep, Period> const& timeout) {
        terminate_internal(dp::chrono::saturating_duration_cast<std::chrono::nanoseconds>(timeout));
    }

public:
    dp::FileDescriptor stdinFd_;
    dp::FileDescriptor stdoutFd_;
    dp::FileDescriptor stderrFd_;

private:
    pid_t pid_;
    int   exitStatus_;
    void  start(std::string application, std::vector<std::string> args, priority::Inheritance pi);
    void  wait_internal(std::chrono::nanoseconds timeout);
    void  terminate_internal(std::chrono::nanoseconds timeout);
};
}   // namespace dp
