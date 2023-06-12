#pragma once

#include "dp_lib/util/log.hpp"

#include <string>
#include <system_error>
#include <utility>

namespace dp {
namespace detail {

    template<typename Exception, typename... Args>
    [[noreturn, gnu::noinline]] void raise(Args&&... args) {
        //NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay, hicpp-no-array-decay)
        throw Exception(std::forward<Args>(args)...);
    }

    template<typename Exception, typename... Args>
    std::string get_what(Args&&... args) {
        //NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay, hicpp-no-array-decay)
        return Exception(std::forward<Args>(args)...).what();
    }

    std::string get_backtrace(std::size_t skip);

}   // namespace detail

#define DPRAISE_PRINT_INTERNAL_WITHOUT_BACK_TRACE(prefix, exception, ...) \
    DPLOG_W(prefix "throwing {} with what(): {}", #exception, dp::detail::get_what<exception>(__VA_ARGS__))

#define DPRAISE_PRINT_INTERNAL_WITH_BACK_TRACE(prefix, exception, ...) \
    DPLOG_W(prefix "throwing {} with what(): {}\n{}",                  \
            #exception,                                                \
            dp::detail::get_what<exception>(__VA_ARGS__),              \
            ::dp::detail::get_backtrace(3))

#ifdef DPLIB_USE_STACKTRACE
    #define DPRAISE_PRINT_INTERNAL(prefix, exception, ...) \
        DPRAISE_PRINT_INTERNAL_WITH_BACK_TRACE(prefix, exception, __VA_ARGS__)
#else
    #define DPRAISE_PRINT_INTERNAL(prefix, exception, ...) \
        DPRAISE_PRINT_INTERNAL_WITHOUT_BACK_TRACE(prefix, exception, __VA_ARGS__)
#endif

#define DPRAISE_PRINT_ONLY(exception, ...) DPRAISE_PRINT_INTERNAL("not ", exception, __VA_ARGS__)

#define DPRAISE(exception, ...)                         \
    DPRAISE_PRINT_INTERNAL("", exception, __VA_ARGS__); \
    dp::detail::raise<exception>(__VA_ARGS__)

#define DPRAISE_MAYBE_SILENT(silent, exception, ...)                      \
    if(!(silent)) { DPRAISE_PRINT_INTERNAL("", exception, __VA_ARGS__); } \
    dp::detail::raise<exception>(__VA_ARGS__)

#define DPRAISE_SYSTEM_ERROR_CE(num, msg) DPRAISE(std::system_error, (num), std::system_category(), (msg))

#define DPRAISE_SYSTEM_ERROR_CE_MAYBE_SILENT(silent, num, msg) \
    DPRAISE_MAYBE_SILENT(silent, std::system_error, (num), std::system_category(), (msg))

#define DPRAISE_SYSTEM_ERROR_CE_PRINT_ONLY(num, msg) \
    DPRAISE_PRINT_ONLY(std::system_error, (num), std::system_category(), (msg))

#define DPRAISE_SYSTEM_ERROR(msg)                                           \
    do {                                                                    \
        auto DPRAISE_DO_NOT_USE_THIS_NAME_ERRNO = errno;                    \
        (void)DPRAISE_DO_NOT_USE_THIS_NAME_ERRNO;                           \
        DPRAISE_SYSTEM_ERROR_CE(DPRAISE_DO_NOT_USE_THIS_NAME_ERRNO, (msg)); \
    } while(false)

#define DPRAISE_SYSTEM_ERROR_MAYBE_SILENT(silent, msg)                                           \
    do {                                                                                         \
        auto DPRAISE_DO_NOT_USE_THIS_NAME_ERRNO = errno;                                         \
        (void)DPRAISE_DO_NOT_USE_THIS_NAME_ERRNO;                                                \
        DPRAISE_SYSTEM_ERROR_CE_MAYBE_SILENT(silent, DPRAISE_DO_NOT_USE_THIS_NAME_ERRNO, (msg)); \
    } while(false)

#define DPRAISE_SYSTEM_ERROR_PRINT_ONLY(msg)                                           \
    do {                                                                               \
        auto DPRAISE_DO_NOT_USE_THIS_NAME_ERRNO = errno;                               \
        (void)DPRAISE_DO_NOT_USE_THIS_NAME_ERRNO;                                      \
        DPRAISE_SYSTEM_ERROR_CE_PRINT_ONLY(DPRAISE_DO_NOT_USE_THIS_NAME_ERRNO, (msg)); \
    } while(false)

}   // namespace dp
