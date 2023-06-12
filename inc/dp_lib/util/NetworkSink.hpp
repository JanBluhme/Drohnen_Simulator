#pragma once

#include "dp_lib/com/IP/Socket.hpp"
#include "dp_lib/com/UNIX/UNIX_Socket.hpp"
#include "dp_lib/util/log.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>

namespace dp { namespace sinks {
    template<typename Mutex, typename Socket>
    class NetworkSink : public spdlog::sinks::base_sink<Mutex> {
    private:
        std::string                           host_;
        std::uint16_t                         port_{};
        std::unique_ptr<Socket>               socket_;
        std::chrono::steady_clock::time_point lastConnectRetry_;
        std::array<std::string_view, 7>       colors_;

    public:
        // Formatting codes
        const std::string_view reset = "\033[m";

        // Foreground colors
        const std::string_view green = "\033[32m";
        const std::string_view cyan  = "\033[36m";
        const std::string_view white = "\033[37m";

        /// Bold colors
        const std::string_view yellow_bold = "\033[33m\033[1m";
        const std::string_view red_bold    = "\033[31m\033[1m";
        const std::string_view bold_on_red = "\033[1m\033[41m";

        NetworkSink(std::string const& host, std::uint16_t port) : NetworkSink{host, port, false} {}

        // TODO add reconnect time?
        NetworkSink(std::string host, std::uint16_t port, bool throwOnFailedConnect)
          : host_{std::move(host)}
          , port_{port}
          , lastConnectRetry_{std::chrono::steady_clock::time_point::min()} {
            colors_[spdlog::level::trace]    = white;
            colors_[spdlog::level::debug]    = cyan;
            colors_[spdlog::level::info]     = green;
            colors_[spdlog::level::warn]     = yellow_bold;
            colors_[spdlog::level::err]      = red_bold;
            colors_[spdlog::level::critical] = bold_on_red;
            colors_[spdlog::level::off]      = reset;

            if(throwOnFailedConnect) {
                reconnect_if_needed_();
            }
        }

    protected:
        void
          sink_it_(spdlog::details::log_msg const& msg) override {
            if(!socket_) {
                reconnect_if_needed_();
            }

            if(socket_) {
                spdlog::memory_buf_t formatted;
                spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

                auto send = [this](spdlog::memory_buf_t const& buff) {
                    try {
                        socket_->send(buff);
                    } catch(...) {
                        reconnect_if_needed_();
                        throw;
                    }
                };

                // need to do this check after format even though msg is const since this two members are mutable.....
                if(msg.color_range_end > msg.color_range_start) {
                    spdlog::memory_buf_t formattedWithColor;
                    fmt::format_to(
                      formattedWithColor,
                      "{}{}{}{}{}",
                      std::string_view{formatted.begin(), msg.color_range_start},
                      //NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                      colors_[msg.level],
                      std::string_view{
                        dp::next(formatted.begin(), msg.color_range_start),
                        msg.color_range_end - msg.color_range_start},
                      reset,
                      std::string_view{
                        dp::next(formatted.data(), msg.color_range_end),
                        formatted.size() - msg.color_range_end});
                    send(formattedWithColor);
                } else {
                    send(formatted);
                }
            }
        }

        void
          flush_() override {}

    private:
        void
          reconnect_if_needed_() {
            if(!socket_ || !socket_->is_valid()) {
                socket_.reset();
                auto const now = std::chrono::steady_clock::now();
                if(now > lastConnectRetry_ + std::chrono::seconds(5)) {
                    lastConnectRetry_ = now;
                    if constexpr(dp::com::isUnixSocket<Socket>()) {
                        socket_ = spdlog::details::make_unique<Socket>(
                          dp::com::Socket::AbstractSocket{host_});
                    } else {
                        socket_ = spdlog::details::make_unique<Socket>(host_, port_);
                    }
                    if(socket_) {
                        socket_->set_send_timeout(std::chrono::milliseconds(500));
                    }
                }
            }
        }
    };

    using tcp_sink_st = NetworkSink<spdlog::details::null_mutex, dp::com::TCP_ClientSocket>;
    using udp_bound_sink_st
      = NetworkSink<spdlog::details::null_mutex, dp::com::UDP_Bound_ClientSocket>;
    using udp_sink_st = NetworkSink<spdlog::details::null_mutex, dp::com::UDP_ClientSocket>;

    using unix_stream_sink_st
      = NetworkSink<spdlog::details::null_mutex, dp::com::UNIX_Stream_ClientSocket>;
    using unix_packet_sink_st
      = NetworkSink<spdlog::details::null_mutex, dp::com::UNIX_Stream_ClientSocket>;

    using sctp_stream_sink_st
      = NetworkSink<spdlog::details::null_mutex, dp::com::SCTP_Stream_ClientSocket>;
    using sctp_packet_sink_st
      = NetworkSink<spdlog::details::null_mutex, dp::com::SCTP_Packet_ClientSocket>;

    using tcp_sink_mt       = NetworkSink<std::mutex, dp::com::TCP_ClientSocket>;
    using udp_bound_sink_mt = NetworkSink<std::mutex, dp::com::UDP_Bound_ClientSocket>;
    using udp_sink_mt       = NetworkSink<std::mutex, dp::com::UDP_ClientSocket>;

    using unix_packet_sink_mt = NetworkSink<std::mutex, dp::com::UNIX_Packet_ClientSocket>;
    using unix_packet_sink_mt = NetworkSink<std::mutex, dp::com::UNIX_Packet_ClientSocket>;

    using sctp_stream_sink_mt = NetworkSink<std::mutex, dp::com::SCTP_Stream_ClientSocket>;
    using sctp_packet_sink_mt = NetworkSink<std::mutex, dp::com::SCTP_Packet_ClientSocket>;

#ifdef DPLIB_USE_SSL
    using ssl_tcp_sink_st = NetworkSink<spdlog::details::null_mutex, dp::com::SSL_TCP_ClientSocket>;
    using ssl_tcp_sink_mt = NetworkSink<std::mutex, dp::com::SSL_TCP_ClientSocket>;
    using ssl_sctp_sink_st
      = NetworkSink<spdlog::details::null_mutex, dp::com::SSL_SCTP_ClientSocket>;
    using ssl_sctp_sink_mt = NetworkSink<std::mutex, dp::com::SSL_SCTP_ClientSocket>;
    using ssl_sink_st      = NetworkSink<spdlog::details::null_mutex, dp::com::SSL_ClientSocket>;
    using ssl_sink_mt      = NetworkSink<std::mutex, dp::com::SSL_ClientSocket>;
#endif

}}   // namespace dp::sinks
