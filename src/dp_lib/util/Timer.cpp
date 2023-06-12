#include "dp_lib/util/Timer.hpp"

#include "dp_lib/util/log.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace dp {

Timer::~Timer() {
    {
        std::lock_guard<std::mutex> lock(mt_);
        killed_ = true;
    }
    cv_.notify_one();
    if(call_thread_.joinable()) { call_thread_.join(); }
}

void Timer::start() {
    {
        std::lock_guard<std::mutex> lock(mt_);
        next_time_ = clock::now() + interval_;
    }
    cv_.notify_one();
}

void Timer::stop() {
    std::lock_guard<std::mutex> lock(mt_);
    next_time_ = clock::time_point::min();
}

void Timer::sleep_and_call() {
    std::unique_lock<std::mutex> lock(mt_);
    while(!killed_) {
        if(next_time_ == clock::time_point::min()) {
            cv_.wait(lock);
        } else {
            auto const now = clock::now();
            if(now >= next_time_) {
                auto const old_next_time = next_time_;
                lock.unlock();
                try {
                    callback_function_();
                } catch(std::exception const& e) {
                    (void)e;
                    DPLOG_C("catched {}", e.what());
                } catch(...) { DPLOG_C("catched ..."); }
                lock.lock();
                if(next_time_ != clock::time_point::min()) {
                    if(mode_ == Timer_Mode::Continuous) {
                        if(old_next_time == next_time_) { next_time_ = now + interval_; }
                    } else {
                        next_time_ = clock::time_point::min();
                    }
                }
            } else {
                cv_.wait_until(lock, next_time_);
            }
        }
    }
}

Pool_Timer_Registry::Pool_Timer_Registry()
  : current_timer_handle_{0}
  , stopped{false}
  , pool_thread_{&Pool_Timer_Registry::pool_runner, this} {}

Pool_Timer_Registry::~Pool_Timer_Registry() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped = true;
    }
    cv_.notify_one();
    if(pool_thread_.joinable()) { pool_thread_.join(); }
}

std::size_t Pool_Timer_Registry::register_pool_timer(clock::duration           interval,
                                                     Timer_Mode                mode,
                                                     std::function<void(void)> callback_function) {
    std::unique_lock<std::mutex> lock(mutex_);

    std::size_t this_handle = ++current_timer_handle_;

    registered_pool_timers_.emplace(
      this_handle,
      Timer_Info{std::move(callback_function), interval, clock::time_point::max(), mode, false});

    lock.unlock();
    cv_.notify_one();
    return this_handle;
}

void Pool_Timer_Registry::deregister_pool_timer(std::size_t handle) {
    if(0 == handle) { return; }
    std::unique_lock<std::mutex> lock(mutex_);

    auto it = registered_pool_timers_.find(handle);
    if(it == registered_pool_timers_.end()) {
        DPLOG_W("Pool_Timer does not exist: {}", handle);
        return;
    }

    if(it->second.currently_running) {
        currently_running_cv_.wait(lock, [&it]() { return !it->second.currently_running; });
    }

    registered_pool_timers_.erase(it);
    lock.unlock();
    cv_.notify_one();
}

void Pool_Timer_Registry::start_pool_timer(std::size_t handle) {
    if(0 == handle) { return; }
    std::unique_lock<std::mutex> lock(mutex_);
    auto                         it = registered_pool_timers_.find(handle);
    if(it == registered_pool_timers_.end()) {
        DPLOG_W("Pool_Timer does not exist: {}", handle);
        return;
    }

    it->second.next_time = clock::now() + it->second.interval;
    lock.unlock();
    cv_.notify_one();
}

void Pool_Timer_Registry::stop_pool_timer(std::size_t handle) {
    if(0 == handle) { return; }
    std::unique_lock<std::mutex> lock(mutex_);
    auto                         it = registered_pool_timers_.find(handle);
    if(it == registered_pool_timers_.end()) {
        DPLOG_W("Pool_Timer does not exist: {}", handle);
        return;
    }

    it->second.next_time = clock::time_point::max();
    lock.unlock();
    cv_.notify_one();
}

void Pool_Timer_Registry::pool_runner() {
    std::unique_lock<std::mutex> lock(mutex_);
    while(!stopped) {
        auto smallest = std::min_element(
          registered_pool_timers_.begin(),
          registered_pool_timers_.end(),
          [](std::pair<std::size_t, Timer_Info> const& lhs, std::pair<std::size_t, Timer_Info> const& rhs) {
              return lhs.second.next_time < rhs.second.next_time;
          });

        if(smallest != registered_pool_timers_.end()) {
            auto const now = clock::now();
            if(now >= smallest->second.next_time) {
                Timer_Info& current       = smallest->second;
                current.currently_running = true;
                auto const old_next_time  = current.next_time;
                lock.unlock();
                try {
                    current.callback_function();
                } catch(std::exception const& e) {
                    (void)e;
                    DPLOG_C("catched {}", e.what());
                } catch(...) { DPLOG_C("catched ..."); }

                lock.lock();
                current.currently_running = false;
                if(current.next_time != clock::time_point::max()) {
                    if(current.mode == Timer_Mode::Continuous) {
                        if(old_next_time == current.next_time) { current.next_time = now + current.interval; }
                    } else {
                        current.next_time = clock::time_point::max();
                    }
                }
                lock.unlock();
                currently_running_cv_.notify_all();
                lock.lock();
            } else {
                if(smallest->second.next_time == clock::time_point::max()) {
                    cv_.wait(lock);
                } else {
                    auto next_time = smallest->second.next_time;
                    cv_.wait_until(lock, next_time);
                }
            }
        } else {
            cv_.wait(lock);
        }
    }
}

Pool_Timer::Pool_Timer(Pool_Timer&& other) noexcept : registry_{other.registry_}, handle_{other.handle_} {
    other.handle_ = 0;
}
Pool_Timer& Pool_Timer::operator=(Pool_Timer&& other) noexcept {
    if(this != std::addressof(other)) {
        registry_     = other.registry_;
        handle_       = other.handle_;
        other.handle_ = 0;
    }
    return *this;
}

Pool_Timer::~Pool_Timer() {
    registry_.get().deregister_pool_timer(handle_);
}

void Pool_Timer::start() {
    registry_.get().start_pool_timer(handle_);
}

void Pool_Timer::stop() {
    registry_.get().stop_pool_timer(handle_);
}

}   // namespace dp
