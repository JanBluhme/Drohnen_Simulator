#include "dp_lib/util/Process.hpp"

#include "dp_lib/util/FileDescriptor.hpp"
#include "dp_lib/util/ScopeGuard.hpp"
// #include "dp_lib/util/byte.hpp"
#include "dp_lib/util/log.hpp"
#include "dp_lib/util/pack.hpp"
#include "dp_lib/util/priority.hpp"
#include "dp_lib/util/raise.hpp"
#include "dp_lib/util/system_call_helper.hpp"

// #include <fmt/format.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <iterator>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace dp {
Process::Process(Process&& other) noexcept
  : stdinFd_{std::move(other.stdinFd_)}
  , stdoutFd_{std::move(other.stdoutFd_)}
  , stderrFd_{std::move(other.stderrFd_)}
  , pid_{other.pid_}
  , exitStatus_{other.exitStatus_} {
    other.pid_ = -1;
}

Process& Process::operator=(Process&& other) noexcept {
    if(this != std::addressof(other)) {
        stdinFd_    = std::move(other.stdinFd_);
        stdoutFd_   = std::move(other.stdoutFd_);
        stderrFd_   = std::move(other.stderrFd_);
        pid_        = other.pid_;
        exitStatus_ = other.exitStatus_;
        other.pid_  = -1;
    }
    return *this;
}

Process::~Process() noexcept {
    if(pid_ > 0) {
        try {
            try {
                wait(std::chrono::milliseconds{500});
            } catch(...) { DPLOG_W("~Process wait timeout"); }
            if(pid_ > 0) {
                DPLOG_W("~Process on running child try to terminate it");
                terminate(std::chrono::seconds(2));
            }
        } catch(std::exception const& e) {
            (void)e;
            DPLOG_C("~Process failed catched {}", e.what());
        } catch(...) { DPLOG_W("~Process failed catched ..."); }
    }
}

void Process::start(std::string application, std::vector<std::string> args, priority::Inheritance pi) {
    std::vector<char*> nonConstArgs;
    nonConstArgs.reserve(args.size() + 1);
    nonConstArgs.push_back(const_cast<char*>(application.c_str()));   //NOLINT(cppcoreguidelines-pro-type-const-cast)
    std::transform(begin(args), end(args), std::back_inserter(nonConstArgs), [](std::string const& arg) {
        return const_cast<char*>(arg.c_str());   //NOLINT(cppcoreguidelines-pro-type-const-cast)
    });
    nonConstArgs.push_back(nullptr);

    auto makePipe = []() {
        std::array<int, 2> fds{};
        if(::pipe2(fds.data(), O_CLOEXEC) == -1) { DPRAISE_SYSTEM_ERROR("pipe2 failed"); }
        return std::pair<dp::FileDescriptor, dp::FileDescriptor>{dp::FileDescriptor{fds[0]},
                                                                 dp::FileDescriptor{fds[1]}};
    };

    auto stdinPipe  = makePipe();
    auto stdoutPipe = makePipe();
    auto stderrPipe = makePipe();

    auto msgPipe    = makePipe();
    auto signalPipe = makePipe();

    enum class ChildError : int {
        dup2Failed,
        execvpFailed,
        priorityFailedWithErrorCode,
        priorityFailed,
    };

    auto getFailString = [](ChildError code) {
        std::string const systemCallFailed{
          code == ChildError::dup2Failed
            ? "dup2"
            : code == ChildError::execvpFailed
                ? "execvp"
                : (code == ChildError::priorityFailed || code == ChildError::priorityFailedWithErrorCode) ? "priority"
                                                                                                          : "unknown"};
        return systemCallFailed;
    };

    pid_ = ::fork();
    if(pid_ == 0) {
        auto transmitError = [&](ChildError code, int err) {
            std::array<std::byte, sizeof(code) + sizeof(err)> buffer{};
             dp::pack(begin(buffer), end(buffer), code, err);
            retry_on_errno<2>(ssize_t{-1}, EINTR, ::write, msgPipe.second.fd(), buffer.data(), buffer.size());
            std::terminate();
        };

        auto doDup2 = [&](int fd1, int fd2) {
            if(-1 == retry_on_errno<2>(-1, EINTR, ::dup2, fd1, fd2)) { transmitError(ChildError::dup2Failed, errno); }
        };

        doDup2(stdinPipe.first.fd(), STDIN_FILENO);
        doDup2(stdoutPipe.second.fd(), STDOUT_FILENO);
        doDup2(stderrPipe.second.fd(), STDERR_FILENO);

        if(pi == priority::Inheritance::Reset) {
            try {
                set_default_scheduler(priority::Inheritance::Inherit);
                try {
                    set_nicenes(priority::Nicenes::Default);
                } catch(...) { set_nicenes(get_current_highest_nicenes()); }
            } catch(std::system_error const& e) {
                transmitError(ChildError::priorityFailedWithErrorCode, e.code().value());
            } catch(...) { transmitError(ChildError::priorityFailed, 0); }
        }

        ::execvp(nonConstArgs.front(), nonConstArgs.data());
        transmitError(ChildError::execvpFailed, errno);

    } else if(pid_ == -1) {
        DPRAISE_SYSTEM_ERROR("fork failed");
    }
    auto const guard =
      make_scope_guard([this]() { terminate(std::chrono::milliseconds(500)); }, ScopeGuardCallPolicy::exception);

    signalPipe.first.close();

    bool sigPipeClosed = false;
    try {
        sigPipeClosed = signalPipe.second.poll(std::chrono::seconds{5}, false, false, false, true);
    } catch(...) { throw; }
    if(!sigPipeClosed) { DPRAISE(std::runtime_error, "start failed"); }
    bool msgPipeRdyRead = false;
    try {
        msgPipeRdyRead = msgPipe.first.can_recv(std::chrono::nanoseconds{0});
    } catch(...) { throw; }

    if(msgPipeRdyRead) {
        std::array<std::byte, sizeof(ChildError) + sizeof(int)> buffer{};
        auto s = retry_on_errno<2>(ssize_t{-1}, EINTR, ::read, msgPipe.first.fd(), buffer.data(), buffer.size());
        if(-1 == s) {
            DPRAISE_SYSTEM_ERROR("read failed");
        } else if(buffer.size() == static_cast<std::size_t>(s)) {
            ChildError code{};
            int        err{};
            dp::unpack(begin(buffer), end(buffer), code, err);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
            //NOLINTNEXTLINE(clang-diagnostic-switch-enum)
            switch(code) {
            case ChildError::priorityFailed: {
                DPRAISE(std::runtime_error, "priority Failed in child");
            }
            default: {
                DPRAISE_SYSTEM_ERROR_CE(err, getFailString(code) + " failed in child");
            }
            }
#pragma GCC diagnostic pop
        } else {
            DPRAISE(std::runtime_error, "start failed");
        }
    } else {
        stdinFd_  = std::move(stdinPipe.second);
        stdoutFd_ = std::move(stdoutPipe.first);
        stderrFd_ = std::move(stderrPipe.first);
    }
}

void Process::signal(int signal) {
    if(pid_ > 0) {
        int ret = ::kill(pid_, signal);
        if(ret == -1) { DPRAISE_SYSTEM_ERROR("signal failed"); }
    } else if(pid_ == 0) {
        DPRAISE(std::logic_error, "signal on already waited process");
    } else {
        DPRAISE(std::logic_error, "signal on moved process");
    }
}

void Process::wait_internal(std::chrono::nanoseconds timeout) {
    if(pid_ > 0) {
        auto const end = std::chrono::steady_clock::now() + timeout;
        while(end >= std::chrono::steady_clock::now()) {
            int         status{};
            pid_t const pid = retry_on_errno<2>(pid_t{-1}, EINTR, ::waitpid, pid_, &status, WNOHANG);
            if(pid == -1) { DPRAISE_SYSTEM_ERROR("waitpid failed"); }
            if(pid == pid_) {
                exitStatus_ = status;
                pid_        = 0;
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));   // TODO signalfd
        }
        DPRAISE(std::runtime_error, "timeout");
    } else if(pid_ == 0) {
        // TODO fail or ok????
        DPRAISE(std::logic_error, "wait on already waited process");
    } else {
        DPRAISE(std::logic_error, "wait on moved process");
    }
}

void Process::terminate_internal(std::chrono::nanoseconds timeout) {
    auto nextSig = [](int oldSig) {
        if(oldSig == SIGINT) { return SIGTERM; }
        return SIGKILL;
    };
    int        sig = SIGINT;
    auto const end = std::chrono::steady_clock::now() + timeout;
    while(pid_ > 0 && end > std::chrono::steady_clock::now()) {
        try {
            signal(sig);
            wait(timeout / 8);
            sig = nextSig(sig);
        } catch(std::exception const& e) {
            (void)e;
            sig = nextSig(sig);
            DPLOG_W("catched {} sending {} now", e.what(), ::strsignal(sig));
        }
    }

    if(pid_ > 0) { DPRAISE(std::runtime_error, "termintate failed"); }
}

bool Process::running() {
    if(pid_ > 0) {
        int   status{};
        pid_t pid = retry_on_errno<2>(pid_t{-1}, EINTR, ::waitpid, pid_, std::addressof(status), WNOHANG);

        if(pid == pid_) {
            exitStatus_ = status;
            pid_        = 0;
            return false;
        }
        if(pid == -1) { DPRAISE_SYSTEM_ERROR("waitpid failed"); }
        return true;
    }
    return false;
}

int Process::exit_status() {
    if(pid_ == 0) {
        return WEXITSTATUS(exitStatus_);   //NOLINT(hicpp-signed-bitwise)
    }
    if(pid_ > 0) {
        DPRAISE(std::logic_error, "exitStatus on not waited process");
    } else {
        DPRAISE(std::logic_error, "exitStatus on moved process");
    }
}
}   // namespace dp
