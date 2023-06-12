#pragma once

#include <functional>

namespace dp {
template<std::size_t count, typename EType, typename F, typename... Args>
EType retry_on_errno(EType errorVal, int errnoRetryValue, F&& f, Args&&... args) {
    std::size_t retrys{};
    while(true) {
        //NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay, hicpp-no-array-decay)
        auto ret = f(std::forward<Args>(args)...);
        if(ret == errorVal) {
            if(errno != errnoRetryValue) { return ret; }
        } else {
            return ret;
        }
        if(retrys == count) { return ret; }
        ++retrys;
    }
}

template<std::size_t count, typename EType, typename F, typename... Args>
EType retry_on_return(EType errorVal, F&& f, Args&&... args) {
    std::size_t retrys{};
    while(true) {
        //NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay, hicpp-no-array-decay)
        auto ret = f(std::forward<Args>(args)...);
        if(ret != errorVal) { return ret; }
        if(retrys == count) { return ret; }
        ++retrys;
    }
}

static inline bool is_errno_recoverable(int errnoToCheck) {
    return errnoToCheck == EAGAIN || (EWOULDBLOCK != EAGAIN && errnoToCheck == EWOULDBLOCK) || errnoToCheck == EINTR;
}

}   // namespace dp
