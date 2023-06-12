#pragma once

#include "dp_lib/util/Monotonic_cv.hpp"

#include <chrono>
#include <cstddef>
#include <functional>
#include <map>
#include <mutex>
#include <thread>

namespace dp {

enum class Timer_Mode { Single_Shot, Continuous };

class Timer {
public:
    using clock = std::chrono::steady_clock;

    template<typename Rep, typename Period, typename Callback_Function>
    Timer(std::chrono::duration<Rep, Period> const& interval, Timer_Mode mode, Callback_Function&& callback_function)
      : callback_function_(callback_function)
      , interval_(interval)
      , next_time_(clock::time_point::min())
      , mode_(mode)
      , killed_(false)
      , call_thread_(&Timer::sleep_and_call, this) {}

    Timer(Timer const&) = delete;
    Timer& operator=(Timer const&) = delete;
    Timer(Timer&&)                 = delete;
    Timer& operator=(Timer&&) = delete;

    ~Timer();

    void start();

    void stop();

private:
    std::function<void(void)> const callback_function_;
    clock::duration const           interval_;
    clock::time_point               next_time_;
    std::mutex                      mt_;
    Monotonic_cv                    cv_;
    Timer_Mode const                mode_;
    bool                            killed_;
    std::thread                     call_thread_;

    void sleep_and_call();
};

class Pool_Timer;

class Pool_Timer_Registry {
public:
    using clock = std::chrono::steady_clock;
    Pool_Timer_Registry();

    Pool_Timer_Registry(Pool_Timer_Registry const&) = delete;
    Pool_Timer_Registry& operator=(Pool_Timer_Registry const&) = delete;

    Pool_Timer_Registry(Pool_Timer_Registry&&) = delete;
    Pool_Timer_Registry& operator=(Pool_Timer_Registry&&) = delete;

    ~Pool_Timer_Registry();

private:
    friend Pool_Timer;

    struct Timer_Info {
        std::function<void(void)> const callback_function;
        clock::duration const           interval;
        clock::time_point               next_time;
        Timer_Mode const                mode;
        bool                            currently_running;
    };

    std::mutex   mutex_;
    Monotonic_cv cv_;
    Monotonic_cv currently_running_cv_;

    std::map<std::size_t, Timer_Info> registered_pool_timers_;

    std::size_t current_timer_handle_;

    bool        stopped;
    std::thread pool_thread_;

    std::size_t
         register_pool_timer(clock::duration interval, Timer_Mode mode, std::function<void(void)> callback_function);
    void deregister_pool_timer(std::size_t handle);

    void start_pool_timer(std::size_t handle);

    void stop_pool_timer(std::size_t handle);

    void pool_runner();
};

class Pool_Timer {
public:
    template<typename Rep, typename Period, typename Callback_Function>
    Pool_Timer(Pool_Timer_Registry&                      registry,
               std::chrono::duration<Rep, Period> const& interval,
               Timer_Mode                                mode,
               Callback_Function&&                       callback_function)
      : registry_{registry}
      , handle_{registry_.get().register_pool_timer(interval, mode, callback_function)} {}

    Pool_Timer(Pool_Timer const&) = delete;
    Pool_Timer& operator=(Pool_Timer const&) = delete;

    Pool_Timer(Pool_Timer&& other) noexcept;
    Pool_Timer& operator=(Pool_Timer&& other) noexcept;

    ~Pool_Timer();

    void start();

    void stop();

private:
    std::reference_wrapper<Pool_Timer_Registry> registry_;
    std::size_t                                 handle_;
};

}   // namespace dp
