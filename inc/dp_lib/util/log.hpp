#pragma once
#include <cstddef>
#include <exception>
#include <iterator>
#include <string>
#include <utility>

namespace CLI {
class App;
}   // namespace CLI

#define DPLOG_LEVEL_TRACE    0
#define DPLOG_LEVEL_DEBUG    1
#define DPLOG_LEVEL_INFO     2
#define DPLOG_LEVEL_WARN     3
#define DPLOG_LEVEL_ERROR    4
#define DPLOG_LEVEL_CRITICAL 5
#define DPLOG_LEVEL_OFF      6

#ifndef DPLOG_ACTIVE_LEVEL
    #define DPLOG_ACTIVE_LEVEL DPLOG_LEVEL_OFF
#endif

#if DPLOG_ACTIVE_LEVEL != DPLOG_LEVEL_OFF
    #include <fmt/format.h>
    #include <fmt/ostream.h>
    #include <fmt/ranges.h>
    #include <string_view>
#endif

namespace dp {
constexpr const char* default_console_pattern{"[%T:%f %-8l] %^%v%$ [%@ %!] [%t]"};
constexpr const char* default_net_pattern{"[%T:%f %-8l] %^%v%$ [%@ %!] [%t]"};
constexpr const char* default_file_pattern{"[%d.%m.%Y %T:%f %-8l] %v [%@ %!] [%t]"};
}   // namespace dp

#define DPLOG_PATTERN ::dp::default_console_pattern

#pragma GCC diagnostic push

#if defined(__clang__) && defined(__clang_minor__)
    #pragma GCC diagnostic ignored "-Wglobal-constructors"
    #pragma GCC diagnostic ignored "-Wdeprecated"
    #pragma GCC diagnostic ignored "-Wweak-vtables"
    #pragma GCC diagnostic ignored "-Wpadded"
    #pragma GCC diagnostic ignored "-Wexit-time-destructors"
    #pragma GCC diagnostic ignored "-Wcovered-switch-default"
#else
    #pragma GCC diagnostic ignored "-Wsign-conversion"
    #pragma GCC diagnostic ignored "-Wnoexcept"
#endif

#if DPLOG_ACTIVE_LEVEL != DPLOG_LEVEL_OFF

    #define SPDLOG_ACTIVE_LEVEL    DPLOG_ACTIVE_LEVEL
    #define SPDLOG_DEFAULT_PATTERN DPLOG_PATTERN

    #include <spdlog/async.h>
    #include <spdlog/sinks/rotating_file_sink.h>
    #include <spdlog/sinks/stdout_color_sinks.h>
    #include <spdlog/sinks/base_sink.h>
    #include <spdlog/spdlog.h>

#endif

#pragma GCC diagnostic pop

namespace CLI {
class App;
}

namespace dp {

void log_level_CLI_parser(CLI::App& app_to_append_to);

enum class loglevel {
    trace    = DPLOG_LEVEL_TRACE,
    debug    = DPLOG_LEVEL_DEBUG,
    info     = DPLOG_LEVEL_INFO,
    warn     = DPLOG_LEVEL_WARN,
    error    = DPLOG_LEVEL_ERROR,
    critical = DPLOG_LEVEL_CRITICAL,
    off      = DPLOG_LEVEL_OFF,
};

#if DPLOG_ACTIVE_LEVEL != DPLOG_LEVEL_OFF
namespace detail {
    //NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    struct Log_Meta_Data {
        loglevel         level;
        std::string_view file;
        int              line;
        std::string_view function;
    };

    void log_internal(Log_Meta_Data const& meta_data, std::string_view msg) noexcept;
    bool should_log(loglevel level) noexcept;
    void log_fallback(Log_Meta_Data const& meta_data, std::string_view what, std::string_view fmt_msg) noexcept;

    template<typename Format, typename... Args>
    void log_CT_checked(Log_Meta_Data const& meta_data, Format&& format_str, Args&&... args) noexcept {
        try {
            fmt::memory_buffer msg_buf;
            fmt::format_to(msg_buf, std::forward<Format>(format_str), std::forward<Args>(args)...);
            detail::log_internal(meta_data, std::string_view(msg_buf.data(), msg_buf.size()));
        } catch(std::exception const& e) {
            detail::log_fallback(meta_data, e.what(), fmt::to_string_view(format_str));
        } catch(...) { detail::log_fallback(meta_data, "no std::exception", fmt::to_string_view(format_str)); }
    }

    void log_RT_checked(Log_Meta_Data const& meta_data, std::string_view format_str, fmt::format_args args) noexcept;

}   // namespace detail
#endif
}   // namespace dp

#if DPLOG_ACTIVE_LEVEL != DPLOG_LEVEL_OFF

    #define DPLOG_STRRCHR(str, sep) static_cast<char const*>(__builtin_strrchr(str, sep))

    #define DPLOG_FILE_BASENAME(file) \
        DPLOG_STRRCHR("/" file, '/') + 1   //NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    #define DPLOG_FUNCTION_MACRO __FUNCTION__

    #define DPLOG_META_DATA_INIZIALIZER(level)                                                           \
        level,                                                                                           \
          std::string_view{                                                                              \
            DPLOG_FILE_BASENAME(__FILE__),                                                               \
            static_cast<std::size_t>(std::distance(std::end(__FILE__), DPLOG_FILE_BASENAME(__FILE__)))}, \
          __LINE__, std::string_view {                                                                   \
            static_cast<char const*>(DPLOG_FUNCTION_MACRO), sizeof(DPLOG_FUNCTION_MACRO) - 1             \
        }

    #pragma GCC diagnostic push

    #if defined(__clang__) && defined(__clang_minor__)
        #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
    #else
        #pragma GCC diagnostic ignored "-Wpedantic"
        #pragma GCC system_header   // why does gcc hate me the pragma above does not work
    #endif

    #define DPLOG_LOG_CALL_RT_CHECKED(level, fmtstring, ...)                                                      \
        do {                                                                                                      \
            if(::dp::detail::should_log(level)) {                                                                 \
                try {                                                                                             \
                    ::dp::detail::log_RT_checked(::dp::detail::Log_Meta_Data{DPLOG_META_DATA_INIZIALIZER(level)}, \
                                                 fmtstring,                                                       \
                                                 fmt::make_format_args(__VA_ARGS__));                             \
                } catch(...) {}                                                                                   \
            }                                                                                                     \
        } while(false)

    #define DPLOG_LOG_CALL_CT_CHECKED(level, fmtstring, ...)                                                      \
        do {                                                                                                      \
            if(::dp::detail::should_log(level)) {                                                                 \
                try {                                                                                             \
                    ::dp::detail::log_CT_checked(::dp::detail::Log_Meta_Data{DPLOG_META_DATA_INIZIALIZER(level)}, \
                                                 FMT_STRING(fmtstring),                                           \
                                                 ##__VA_ARGS__);                                                  \
                } catch(...) {}                                                                                   \
            }                                                                                                     \
        } while(false)

    #pragma GCC diagnostic pop

    #ifdef DPLOG_CHECK_FORMAT_STRINGS
        #define DPLOG_LOG(level, ...) DPLOG_LOG_CALL_CT_CHECKED(level, __VA_ARGS__)
    #else
        #define DPLOG_LOG(level, ...)                                                  \
            _Pragma("GCC diagnostic push");                                            \
            _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\""); \
            DPLOG_LOG_CALL_RT_CHECKED(level, __VA_ARGS__);                             \
            _Pragma("GCC diagnostic pop");                                             \
            do {                                                                       \
            } while(false)
    #endif
#else
    #define DPLOG_LOG(level, ...) (void)0
#endif

#if DPLOG_ACTIVE_LEVEL <= DPLOG_LEVEL_TRACE
    #define DPLOG_TRACE(...) DPLOG_LOG(dp::loglevel::trace, __VA_ARGS__)
#else
    #define DPLOG_TRACE(...) (void)0
#endif

#if DPLOG_ACTIVE_LEVEL <= DPLOG_LEVEL_DEBUG
    #define DPLOG_DEBUG(...) DPLOG_LOG(dp::loglevel::debug, __VA_ARGS__)
#else
    #define DPLOG_DEBUG(...) (void)0
#endif

#if DPLOG_ACTIVE_LEVEL <= DPLOG_LEVEL_INFO
    #define DPLOG_INFO(...) DPLOG_LOG(dp::loglevel::info, __VA_ARGS__)
#else
    #define DPLOG_INFO(...) (void)0
#endif

#if DPLOG_ACTIVE_LEVEL <= DPLOG_LEVEL_WARN
    #define DPLOG_WARN(...) DPLOG_LOG(dp::loglevel::warn, __VA_ARGS__)
#else
    #define DPLOG_WARN(...) (void)0
#endif

#if DPLOG_ACTIVE_LEVEL <= DPLOG_LEVEL_ERROR
    #define DPLOG_ERROR(...) DPLOG_LOG(dp::loglevel::error, __VA_ARGS__)
#else
    #define DPLOG_ERROR(...) (void)0
#endif

#if DPLOG_ACTIVE_LEVEL <= DPLOG_LEVEL_CRITICAL
    #define DPLOG_CRITICAL(...) DPLOG_LOG(dp::loglevel::critical, __VA_ARGS__)
#else
    #define DPLOG_CRITICAL(...) (void)0
#endif

#define DPLOG_T(...) DPLOG_TRACE(__VA_ARGS__)
#define DPLOG_D(...) DPLOG_DEBUG(__VA_ARGS__)
#define DPLOG_I(...) DPLOG_INFO(__VA_ARGS__)
#define DPLOG_W(...) DPLOG_WARN(__VA_ARGS__)
#define DPLOG_E(...) DPLOG_ERROR(__VA_ARGS__)
#define DPLOG_C(...) DPLOG_CRITICAL(__VA_ARGS__)
