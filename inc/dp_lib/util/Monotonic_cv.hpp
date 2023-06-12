#pragma once

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <mutex>
#include <pthread.h>

namespace dp {

class Monotonic_cv {
    using _clock_t     = std::chrono::steady_clock;
    using _native_type = pthread_cond_t;
    _native_type _M_cond;

    static_assert(_clock_t::is_steady, "std::chrono::steady_clock is not steady");

public:
    using native_handle_type = _native_type*;

    Monotonic_cv();
    ~Monotonic_cv() noexcept;

    Monotonic_cv(Monotonic_cv const&) = delete;
    Monotonic_cv& operator=(Monotonic_cv const&) = delete;
    Monotonic_cv(Monotonic_cv&&)                 = delete;
    Monotonic_cv& operator=(Monotonic_cv&&) = delete;

    void notify_one();

    void notify_all();

    void wait(std::unique_lock<std::mutex>& _lock);

    template<typename _Predicate>
    void wait(std::unique_lock<std::mutex>& _lock, _Predicate _p) {
        while(!_p()) { wait(_lock); }
    }

    template<typename _Duration>
    std::cv_status wait_until(std::unique_lock<std::mutex>&                       _lock,
                              std::chrono::time_point<_clock_t, _Duration> const& _atime) {
        return _wait_until_impl(_lock, _atime);
    }

    template<typename _Clock, typename _Duration>
    std::cv_status wait_until(std::unique_lock<std::mutex>&                     _lock,
                              std::chrono::time_point<_Clock, _Duration> const& _atime) {
        // DR 887 - Sync unknown clock to known clock.
        typename _Clock::time_point const _c_entry = _Clock::now();
        _clock_t::time_point const        _s_entry = _clock_t::now();
        auto const                        _delta   = _atime - _c_entry;
        auto const                        _s_atime = _s_entry + _delta;

        return _wait_until_impl(_lock, _s_atime);
    }

    template<typename _Clock, typename _Duration, typename _Predicate>
    bool wait_until(std::unique_lock<std::mutex>&                     _lock,
                    std::chrono::time_point<_Clock, _Duration> const& _atime,
                    _Predicate                                        _p) {
        while(!_p()) {
            if(wait_until(_lock, _atime) == std::cv_status::timeout) { return _p(); }
        }
        return true;
    }

    template<typename _Rep, typename _Period>
    std::cv_status wait_for(std::unique_lock<std::mutex>& _lock, std::chrono::duration<_Rep, _Period> const& _rtime) {
        using _dur    = typename _clock_t::duration;
        auto _reltime = std::chrono::duration_cast<_dur>(_rtime);
        if(_reltime < _rtime) { ++_reltime; }
        return wait_until(_lock, _clock_t::now() + _reltime);
    }

    template<typename _Rep, typename _Period, typename _Predicate>
    bool
      wait_for(std::unique_lock<std::mutex>& _lock, std::chrono::duration<_Rep, _Period> const& _rtime, _Predicate _p) {
        using _dur    = typename _clock_t::duration;
        auto _reltime = std::chrono::duration_cast<_dur>(_rtime);
        if(_reltime < _rtime) { ++_reltime; }
        return wait_until(_lock, _clock_t::now() + _reltime, std::move(_p));
    }

    native_handle_type native_handle() { return &_M_cond; }

private:
    template<typename _Dur>
    std::cv_status _wait_until_impl(std::unique_lock<std::mutex>&                  _lock,
                                    std::chrono::time_point<_clock_t, _Dur> const& _atime) {
        auto _s  = std::chrono::time_point_cast<std::chrono::seconds>(_atime);
        auto _ns = std::chrono::duration_cast<std::chrono::nanoseconds>(_atime - _s);

        struct timespec _ts {};
        _ts.tv_sec  = static_cast<decltype(_ts.tv_sec)>(_s.time_since_epoch().count());
        _ts.tv_nsec = static_cast<decltype(_ts.tv_nsec)>(_ns.count());

        pthread_cond_timedwait(&_M_cond, _lock.mutex()->native_handle(), &_ts);

        return (_clock_t::now() < _atime ? std::cv_status::no_timeout : std::cv_status::timeout);
    }
};
}   // namespace dp
