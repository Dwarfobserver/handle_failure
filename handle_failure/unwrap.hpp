
// Implements hf::unwrap and HF_UNWRAP which is used as hf::handle_failure.
// It will throw an exception in case of failure.
// It takes a lambda to only create the context informations in case of failure. 
//
// Usage :
// f(arg) >> hf::unwrap([&] (auto&& f) { f("While calling f(", arg.name(), ")"); });
// Equivalent :
// f(arg) >> HF_UNWRAP("While calling f(", arg.name(), ")");

#pragma once

#include <handle_failure/core.hpp>
#include <sstream>

namespace hf {

// The exception thown by the 'unwrap' function in case of failure.
class unwrap_error : public std::runtime_error {
public:
    unwrap_error(std::string const& str) : std::runtime_error{str} {}
};

namespace detail {
    struct formatter {
        template <class ErrorInfo, class ContextProducer>
        [[noreturn]]
        void operator()(ErrorInfo&& info, ContextProducer&& f) const {
            auto ss = std::ostringstream{};
            ss << "Error message : " << info << ". ";
            f([&] (auto&&...strs) {
                if constexpr (sizeof...(strs) > 0) {
                    ss << "Context info : ";
                    (ss << ... << std::forward<decltype(strs)>(strs));
                    ss << '.';
                }
            });
            throw unwrap_error{ ss.str() };
        }
    };
} // ::detail

template <class ContextProducer>
constexpr auto unwrap(ContextProducer&& f) noexcept {
    return handle_failure(detail::formatter{}, std::forward<ContextProducer>(f));
}
constexpr auto unwrap() noexcept {
    return unwrap([] (auto&&) {});
}

#define HF_UNWRAP(...) ::hf::unwrap([&] (auto&& _hf_formatter_) { _hf_formatter_(__VA_ARGS__); })

} // ::hf
