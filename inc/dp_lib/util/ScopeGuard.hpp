#pragma once

#include "dp_lib/util/log.hpp"

#include <exception>
#include <functional>

namespace dp {

enum class ScopeGuardCallPolicy { always, no_exception, exception, never };
template<typename F>
struct ScopeGuard {
public:
    ScopeGuard(F&& f, ScopeGuardCallPolicy p) : f_{std::forward<F>(f)}, policy_ { p }
#ifdef __cpp_lib_uncaught_exceptions
    , exceptionsAtStart_ { std::uncaught_exceptions() }
#endif
    {}

    ScopeGuard(ScopeGuard const& other) = delete;
    ScopeGuard& operator=(ScopeGuard const& other) = delete;
    ScopeGuard(ScopeGuard&& other) noexcept        = default;
    ScopeGuard& operator=(ScopeGuard&& other) noexcept = default;

    ~ScopeGuard() noexcept {
        if(policy_ != ScopeGuardCallPolicy::never
           && (policy_ == ScopeGuardCallPolicy::always
               || (
#ifdef __cpp_lib_uncaught_exceptions
                 (std::uncaught_exceptions() > exceptionsAtStart_)
#else
                 std::uncaught_exception()
#endif
                 && (policy_ == ScopeGuardCallPolicy::exception)))) {
            try {
                f_();
            } catch(std::exception const& e) {
                (void)e;
                DPLOG_C("catched {}", e.what());
            } catch(...) { DPLOG_C("catched ..."); }
        }
    }

    void setPolicy(ScopeGuardCallPolicy p) noexcept { policy_ = p; }

private:
    F                    f_;
    ScopeGuardCallPolicy policy_;
#ifdef __cpp_lib_uncaught_exceptions
    int exceptionsAtStart_;
#endif
};

template<typename F>
ScopeGuard<typename std::decay<F>::type> make_scope_guard(F&& f, ScopeGuardCallPolicy p) {
    return {std::forward<F>(f), p};
}
}   // namespace dp
