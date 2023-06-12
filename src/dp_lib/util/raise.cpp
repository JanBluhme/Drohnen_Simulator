#include "raise.hpp"

#include <fmt/format.h>

#ifdef DPLIB_USE_STACKTRACE

    #pragma GCC diagnostic push

    #pragma GCC diagnostic ignored "-Wsign-conversion"
    #pragma GCC diagnostic ignored "-Wstrict-aliasing"

    #if defined(__clang__) && defined(__clang_minor__)
        #pragma GCC diagnostic ignored "-Wold-style-cast"
        #pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
        #pragma GCC diagnostic ignored "-Wextra-semi-stmt"
        #pragma GCC diagnostic ignored "-Wexit-time-destructors"
        #pragma GCC diagnostic ignored "-Wweak-vtables"
    #else

    #endif

    #include <backward.hpp>
    #pragma GCC diagnostic pop
#endif

#include <string>

namespace dp { namespace detail {
    std::string get_backtrace(std::size_t skip) {
#ifdef DPLIB_USE_STACKTRACE
        backward::StackTrace st;
        st.load_here();
        st.skip_n_firsts(skip);
        backward::TraceResolver tr;
        tr.load_stacktrace(st);

        fmt::memory_buffer buff;
        fmt::format_to(buff, "stacktrace:");

        for(std::size_t i{}; i < st.size(); ++i) {
            auto const trace = tr.resolve(st[i]);

            std::string filename{trace.source.filename};
            if(filename.empty()) {
                filename = trace.object_filename;
            } else {
                filename = "/" + filename;
                auto pos = filename.find_last_of("/");
                filename = filename.substr(pos + 1);
            }
            fmt::format_to(buff,
                           "\n#{} {}{}:{}{} {}{}",
                           i,
                           "\033[32m",
                           filename,
                           trace.source.line,
                           "\033[35m",
                           trace.object_function,
                           "\033[33m\033[1m");
        }

        return to_string(buff);
#else
        (void)skip;
        return {};
#endif
    }

}}   // namespace dp::detail
