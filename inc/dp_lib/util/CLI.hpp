#pragma once

#pragma GCC diagnostic push
#if defined(__clang__) && defined(__clang_minor__)
    #pragma GCC diagnostic ignored "-Wextra-semi"
    #pragma GCC diagnostic ignored "-Wexit-time-destructors"
    #pragma GCC diagnostic ignored "-Wpadded"
    #pragma GCC diagnostic ignored "-Wglobal-constructors"
    #pragma GCC diagnostic ignored "-Wdocumentation"
    #pragma GCC diagnostic ignored "-Wcovered-switch-default"
    #pragma GCC diagnostic ignored "-Wweak-vtables"
#else
    #pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#endif

#include <CLI/CLI.hpp>

#pragma GCC diagnostic pop

#include "dp_lib/util/log.hpp"

#include <functional>

namespace CLI {
template<typename S, typename F, typename... ParseArgs>
bool save_setup_parse_print(S&& description, F&& f, ParseArgs&&... pargs) noexcept {
    try {
        //NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay, hicpp-no-array-decay)
        CLI::App app(std::forward<S>(description));
        f(app);
        try {
            app.parse(std::forward<ParseArgs>(pargs)...);
        } catch(CLI::ParseError const& e) {
            if(e.get_exit_code() != static_cast<int>(CLI::ExitCodes::Success)) {
                DPLOG_C("\"{}\" Invalid command line try -h", e.what());
                //NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
                std::fprintf(stderr, "\"%s\" Invalid command line try -h\n", e.what());
            }
            try {
                std::puts(app.help().c_str());
            } catch(...) {}
            return false;
        }
    } catch(...) {
        DPLOG_C("Invalid CLI setup");
        return false;
    }
    return true;
}
}   // namespace CLI
