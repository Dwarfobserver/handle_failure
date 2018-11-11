
// Defines the basic interface to handle failures and the traits class to specialize for results
// can that be handled.

#pragma once

#include <utility>
#include <tuple>
#include <cstddef>

namespace hf {

// +-----------+
// | Interface |
// +-----------+

// Calls f(error_info, args...) on failure and returns the unwrapped result (if there is one).
// Usage : f() >> handle_failure(handler, args...);
//
// Do NOT store this function result, as it does not preserve temporary 'args...' lifetime.
template <class FailureHandler, class...Args>
constexpr auto handle_failure(FailureHandler&& f, Args&&...args) noexcept;

// Structure to specialize to handle the error represented by T and optionally retrieve it's value(s).
template <class T>
struct error_traits;
// {
    // Indicates if the result value contains a value (appart from the handled error).
    // static constexpr bool contains_value;

    // If 'contains_value' is true :
    // Returns the value(s) contained. Garanteed to be called when 'indicates_error' returns false.
    // static auto extract_value(T&&);

    // Indicates if the result value indicates a failure.
    // static bool indicates_error(T const&);
    
    // Returns the first argument passed to the TextFormatter, generally a string-like type.
    // static auto error_info(T const&);
// };

// +----------------+
// | Implementation |
// +----------------+

#ifdef _MSC_VER
#define HF_NEVER_INLINE_ __declspec(noinline)
#else
#define HF_NEVER_INLINE_ [[gnu::noinline]]
#endif

#ifdef _MSC_VER
#define HF_UNLIKELY_(x) (static_cast<bool>(x))
#else
#define HF_UNLIKELY_(x) (__builtin_expect(static_cast<bool>(x), false))
#endif

namespace detail {
    template <class FailureHandler, class...Args>
    struct failure_context_t :
        FailureHandler,
        std::tuple<Args...>
    {
        template <class T, size_t...Is>
        void trigger_failure(T const& value, std::index_sequence<Is...>) {
            auto handler = static_cast<FailureHandler&>(*this);
            auto context = static_cast<std::tuple<Args...>&&>(*this);
            handler(error_traits<T>::error_info(value), std::forward<Args>(std::get<Is>(context))...);
        }
        template <class T>
        HF_NEVER_INLINE_
        void trigger_failure(T const& value) {
            using seq = std::make_index_sequence<sizeof...(Args)>;
            trigger_failure(value, seq{});
        }
    };

    // Take parameters by value to benefits from RVO.
    template <class T, class...ContextTs>
    constexpr auto operator>>(T value, failure_context_t<ContextTs...> context) {
        using traits = error_traits<T>;
        if HF_UNLIKELY_ (traits::indicates_error(value)) {
            context.trigger_failure(value);
        }
        if constexpr (traits::contains_value) {
            return traits::extract_value(std::move(value));
        }
    }
}

template <class FailureHandler, class...Args>
constexpr auto handle_failure(FailureHandler&& handler, Args&&...args) noexcept {
    // Temporary failure handlers are stored by value as they often are empty function objects.
    using context_t = detail::failure_context_t<FailureHandler, Args&&...>;
    return context_t{ std::forward<FailureHandler>(handler), { std::forward<Args>(args)... } };
}

} // ::hf
