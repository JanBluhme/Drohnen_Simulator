#include "dp_lib/util/log.hpp"

#include "dp_lib/util/print.hpp"
#include "dp_lib/util/string.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#if DPLOG_ACTIVE_LEVEL != DPLOG_LEVEL_OFF

    #include "dp_lib/util/NetworkSink.hpp"

namespace dp { namespace detail {
    void log_fallback(Log_Meta_Data const& meta_data, std::string_view what, std::string_view fmt_msg) noexcept {
        try {
            //NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
            std::fprintf(stderr,
                         "LOG FAILED!!! level[%i] [%.*s:%i %.*s] with \"%.*s\" fmt/msg was \"%.*s\"\n",
                         static_cast<int>(meta_data.level),
                         static_cast<int>(meta_data.file.size()),
                         meta_data.file.data(),
                         meta_data.line,
                         static_cast<int>(meta_data.function.size()),
                         meta_data.function.data(),
                         static_cast<int>(what.size()),
                         what.data(),
                         static_cast<int>(fmt_msg.size()),
                         fmt_msg.data());
        } catch(...) {}
    }

    void log_RT_checked(Log_Meta_Data const& meta_data, std::string_view format_str, fmt::format_args args) noexcept {
        try {
            fmt::memory_buffer msg_buf;
            fmt::vformat_to(msg_buf, format_str, args);
            detail::log_internal(meta_data, std::string_view(msg_buf.data(), msg_buf.size()));
        } catch(std::exception const& e) { detail::log_fallback(meta_data, e.what(), format_str); } catch(...) {
            detail::log_fallback(meta_data, "no std::exception", format_str);
        }
    }

    void log_internal(Log_Meta_Data const& meta_data, std::string_view msg) noexcept {
        try {
            auto logger = spdlog::default_logger();
            if(!logger) { return; }

            auto lvl = static_cast<spdlog::level::level_enum>(meta_data.level);
            logger->log(spdlog::source_loc{meta_data.file.data(), meta_data.line, meta_data.function.data()}, lvl, msg);
        } catch(std::exception const& e) { log_fallback(meta_data, e.what(), msg); } catch(...) {
            log_fallback(meta_data, "no std::exception", msg);
        }
    }   // namespace detail

    bool should_log(loglevel level) noexcept {
        try {
            auto logger = spdlog::default_logger();
            if(!logger) { return false; }

            auto lvl = static_cast<spdlog::level::level_enum>(level);
            return logger->should_log(lvl);
        } catch(...) { return false; }
    }
}}   // namespace dp::detail
#endif
