#pragma once

#include <fmt/format.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wshadow"

#include <indicators/progress_bar.hpp>

#pragma GCC diagnostic pop

#include <optional>
#include <string>

namespace dp {
struct ProgressBar {
    std::size_t             size;
    std::size_t             current;
    std::string             prefix;
    std::string             postfix;
    indicators::ProgressBar bar;

    ProgressBar(std::size_t size_, std::string_view prefix_, std::string_view postfix_)
      : size{size_}
      , current{}
      , prefix{prefix_}
      , postfix{postfix_}
      , bar{indicators::option::ShowPercentage{true},
            indicators::option::MaxProgress{size + 1},
            indicators::option::Start{"["},
            indicators::option::Fill{"="},
            indicators::option::Lead{">"},
            indicators::option::Remainder{"."},
            indicators::option::End{"]"},
            indicators::option::BarWidth{24U},
            indicators::option::ShowRemainingTime{true},
            indicators::option::PostfixText{postfix}} {
        setPrefix();
    }

    void tick() {
        ++current;
        setPrefix();
        bar.tick();
    }

    void finalize(std::string message) {
        bar.set_option(indicators::option::ShowRemainingTime{false});
        bar.set_option(indicators::option::ShowElapsedTime{true});
        bar.set_option(indicators::option::PostfixText(postfix + " " + message));
        if(!bar.is_completed()) { bar.set_progress(size + 1); }
    }

private:
    void setPrefix() {
        auto const decSize = std::to_string(size).size();
        bar.set_option(indicators::option::PrefixText{fmt::format("{0} {1:>{3}}/{2}", prefix, current, size, decSize)});
    }
};
}   // namespace dp
