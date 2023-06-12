#include "dp_lib/util/log.hpp"
#include "dp_lib/util/CLI.hpp"
#include "dp_lib/util/print.hpp"
#include "dp_lib/util/string.hpp"
#include "dp_lib/util/NetworkSink.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace dp {
namespace {

    enum class NWLogType { udp, tcp, unix };

    struct GeneralLogState {
        std::string  pattern;
        dp::loglevel minLevel{static_cast<dp::loglevel>(DPLOG_ACTIVE_LEVEL)};
    };

    struct NWLogCLIState {
        GeneralLogState glState;
        NWLogType       type{NWLogType::udp};
        std::uint16_t   port{4545};
        std::string     server{"localhost"};
        bool            enable{false};
    };

    struct FLogCLIState {
        GeneralLogState glState;
        std::size_t     files{3};
        std::size_t     fileSize{1048576 * 100};
        std::string     filename;
        bool            enable{false};
    };

    struct LogCLIState {
        NWLogCLIState   nwState;
        FLogCLIState    fState;
        GeneralLogState glState;
        bool            disable{false};
        bool            consoleOverride{false};
    };

    CLI::Option* add_min_level_option(CLI::App& app, dp::loglevel& minLevel) {
        std::array<std::pair<std::string, loglevel>, 7> const logLevelMapAll{{{"trace", loglevel::trace},
                                                                              {"debug", loglevel::debug},
                                                                              {"info", loglevel::info},
                                                                              {"warn", loglevel::warn},
                                                                              {"error", loglevel::error},
                                                                              {"critical", loglevel::critical},
                                                                              {"off", loglevel::off}}};

        std::vector<std::pair<std::string, loglevel>> logLevelMapActive{};
        std::copy_if(begin(logLevelMapAll),
                     end(logLevelMapAll),
                     std::back_inserter(logLevelMapActive),
                     [&minLevel](std::pair<std::string, loglevel> const& l) { return l.second >= minLevel; });

        std::vector<std::pair<std::string, loglevel>> logLevelMapTranform{};

        std::transform(begin(logLevelMapAll),
                       end(logLevelMapAll),
                       std::back_inserter(logLevelMapTranform),
                       [&minLevel](std::pair<std::string, loglevel> const& l) {
                           return std::pair<std::string, loglevel>{l.first, std::max(minLevel, l.second)};
                       });

        return app.add_option("-m,--min-level", minLevel, "the minimum severity level which should get logged.", true)
          ->transform(CLI::Transformer(logLevelMapTranform, CLI::ignore_case).description(""))
          ->check(CLI::Transformer(logLevelMapActive, CLI::ignore_case))
          //NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
          ->default_str(logLevelMapAll[static_cast<std::size_t>(minLevel)].first);
    }

    CLI::Option* add_pattern_option(CLI::App& app, std::string& pattern, std::string const& defaultPattern) {
        return app.add_option("-l,--log-pattern", pattern, "the log pattern")->default_str(defaultPattern);
    }

    void NW_log_level_CLI_parser(NWLogCLIState& st, CLI::App& app_to_append_to) {
        auto app       = app_to_append_to.add_subcommand("net", "commands to control network log");
        auto enableOpt = app->add_flag("-e,--enable", st.enable, "enable network logger");

        add_min_level_option(*app, st.glState.minLevel)->needs(enableOpt);
        add_pattern_option(*app, st.glState.pattern, string::quoted(default_net_pattern))->needs(enableOpt);

        app->add_option("-p,--port", st.port, "ip port to use", true)->needs(enableOpt);

        app->add_option("-s,--server", st.server, "the server to send the logs to", true)->needs(enableOpt);

        std::array<std::pair<std::string, NWLogType>, 3> const NWLogTypeMap{
          {{"udp", NWLogType::udp}, {"tcp", NWLogType::tcp}, {"unix", NWLogType::unix}}};

        app->add_option("-t,--type", st.type, "ip protocol to use", true)
          ->needs(enableOpt)
          ->transform(CLI::Transformer(NWLogTypeMap, CLI::ignore_case))
          //NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
          ->default_str(NWLogTypeMap[static_cast<std::size_t>(st.type)].first);
    }

    void F_log_level_CLI_parser(FLogCLIState& st, CLI::App& app_to_append_to) {
        auto app       = app_to_append_to.add_subcommand("file", "commands to control file log");
        auto enableOpt = app->add_flag("-e,--enable", st.enable, "enable file logger");

        add_min_level_option(*app, st.glState.minLevel)->needs(enableOpt);
        add_pattern_option(*app, st.glState.pattern, string::quoted(default_file_pattern))->needs(enableOpt);

        auto filenameOpt = app->add_option("-f,--filename", st.filename, "name of the log file")->needs(enableOpt);

        app->add_option("-s,--filesize", st.fileSize, "the size of 1 log file", true)
          ->needs(enableOpt)
          //->check(CLI::Range(std::numeric_limits<std::size_t>::min(),std::numeric_limits<std::size_t>::max()))
          ->transform(CLI::AsSizeValue{false});

        app->add_option("-n,--files", st.files, "the amount of log files", true)->needs(enableOpt);

        enableOpt->needs(filenameOpt);
    }
#if DPLOG_ACTIVE_LEVEL != DPLOG_LEVEL_OFF
    template<typename Sink>
    void add_init_sink(GeneralLogState const& st, Sink& sink, std::string const& defaultPattern) {
        auto const& pattern = st.pattern.empty() ? defaultPattern : st.pattern;
        sink->set_pattern(pattern);

        auto level(static_cast<spdlog::level::level_enum>(st.minLevel));

        auto defaultLogger = spdlog::default_logger();

        auto oldLevel = defaultLogger->level();
        defaultLogger->set_level(std::min(level, oldLevel));
        sink->set_level(level);
        defaultLogger->sinks().push_back(sink);
    }

    void NW_final_callback(NWLogCLIState const& st) {
        auto sink = [&]() -> spdlog::sink_ptr {
            if(st.type == NWLogType::tcp) { return std::make_shared<dp::sinks::tcp_sink_st>(st.server, st.port); }
            if(st.type == NWLogType::udp) { return std::make_shared<dp::sinks::udp_sink_st>(st.server, st.port); }
            return std::make_shared<dp::sinks::unix_packet_sink_st>(st.server, st.port);
        }();
        add_init_sink(st.glState, sink, default_net_pattern);
    }

    void F_final_callback(FLogCLIState const& st) {
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(st.filename, st.fileSize, st.files);
        add_init_sink(st.glState, sink, default_file_pattern);
    }
#endif
    void log_final_callback(LogCLIState const& st) {
#if DPLOG_ACTIVE_LEVEL != DPLOG_LEVEL_OFF
        if(st.disable) { spdlog::drop_all(); }

        if(st.consoleOverride) {
            spdlog::drop_all();
            spdlog::set_automatic_registration(false);
            auto logger = spdlog::stderr_color_mt("");
            spdlog::set_automatic_registration(true);

            auto const& pattern = st.glState.pattern.empty() ? default_console_pattern : st.glState.pattern;
            logger->set_pattern(pattern);

            logger->set_level(static_cast<spdlog::level::level_enum>(st.glState.minLevel));

            spdlog::set_default_logger(logger);
        }

        spdlog::set_level(static_cast<spdlog::level::level_enum>(st.glState.minLevel));

        if(!st.glState.pattern.empty()) { spdlog::set_pattern(st.glState.pattern); }

        if(st.nwState.enable || st.fState.enable) {
            spdlog::drop_all();
            // do not use more then 1 thread or make sinks _mt and not _st
            spdlog::init_thread_pool(1024 * 16, 1);
            auto const                    level(static_cast<spdlog::level::level_enum>(st.glState.minLevel));
            std::vector<spdlog::sink_ptr> emptySinks;

            auto logger = std::make_shared<spdlog::async_logger>("",
                                                                 emptySinks.begin(),
                                                                 emptySinks.end(),
                                                                 spdlog::thread_pool(),
                                                                 spdlog::async_overflow_policy::overrun_oldest);
            logger->set_level(level);

            bool addStdErrSink = !st.disable;

            if(addStdErrSink) {
                auto sink = std::make_shared<spdlog::sinks::stderr_color_sink_st>();
                sink->set_level(level);
                auto const& pattern = st.glState.pattern.empty() ? default_console_pattern : st.glState.pattern;
                sink->set_pattern(pattern);
                logger->sinks().push_back(sink);
            }

            spdlog::set_default_logger(logger);
            ::atexit(spdlog::shutdown);
        }

        if(st.nwState.enable) { NW_final_callback(st.nwState); }

        if(st.fState.enable) { F_final_callback(st.fState); }
#else
        (void)st;
#endif
    }

}   // namespace

void log_level_CLI_parser(CLI::App& app_to_append_to) {
    auto logState = std::make_shared<LogCLIState>();

    auto app        = app_to_append_to.add_subcommand("log", "commands to control log")->configurable();
    auto disableOpt = app->add_flag("-d,--disable", logState->disable, "disable default logger");
    add_min_level_option(*app, logState->glState.minLevel)->excludes(disableOpt);
    add_pattern_option(*app, logState->glState.pattern, string::quoted(DPLOG_PATTERN))->excludes(disableOpt);

    NW_log_level_CLI_parser(logState->nwState, *app);
    F_log_level_CLI_parser(logState->fState, *app);

    auto log_final_callback_ = [logState]() { log_final_callback(*logState); };
    app->final_callback(log_final_callback_);
}
}   // namespace dp
