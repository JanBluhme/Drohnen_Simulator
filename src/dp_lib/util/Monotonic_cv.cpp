#include "dp_lib/util/Monotonic_cv.hpp"

#include "dp_lib/util/log.hpp"
#include "dp_lib/util/raise.hpp"

#include <sys/time.h>

namespace dp {
Monotonic_cv::Monotonic_cv() : _M_cond(PTHREAD_COND_INITIALIZER) {
    pthread_condattr_t attr;
    int                rc = pthread_condattr_init(&attr);
    if(rc != 0) { DPRAISE_SYSTEM_ERROR_CE(rc, "pthread_condattr_init failed"); }
    rc = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    if(rc != 0) { DPRAISE_SYSTEM_ERROR_CE(rc, "pthread_condattr_setclock failed"); }
    rc = pthread_cond_init(&_M_cond, &attr);
    if(rc != 0) { DPRAISE_SYSTEM_ERROR_CE(rc, "pthread_cond_init failed"); }
}

Monotonic_cv::~Monotonic_cv() noexcept {
    int const rc = pthread_cond_destroy(&_M_cond);
    if(rc != 0) { DPLOG_C("pthread_cond_destroy failed with {}", rc); }
}

void Monotonic_cv::notify_one() {
    int const rc = pthread_cond_signal(&_M_cond);
    if(rc != 0) { DPRAISE_SYSTEM_ERROR_CE(rc, "pthread_cond_signal failed"); }
}

void Monotonic_cv::notify_all() {
    int const rc = pthread_cond_broadcast(&_M_cond);
    if(rc != 0) { DPRAISE_SYSTEM_ERROR_CE(rc, "pthread_cond_broadcast failed"); }
}

void Monotonic_cv::wait(std::unique_lock<std::mutex>& lock) {
    int const rc = pthread_cond_wait(&_M_cond, lock.mutex()->native_handle());
    if(rc != 0) { DPRAISE_SYSTEM_ERROR_CE(rc, "pthread_cond_wait failed"); }
}
}   // namespace dp
