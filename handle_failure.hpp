
// Defines the basic interface to handle failures and the traits class to specialize for results
// can that be handled.

#pragma once

#include <utility>
#include <tuple>
#include <cstddef>
#include <optional>
#include <system_error>
#include <string>
#include <string_view>
#include <sstream>

namespace hf {

// +-----------+
// | Interface |
// +-----------+

// Calls 'handler(error_info, args...)' on failure and returns the unwrapped result (if there is one).
// Usage : f() >> handle_failure(handler, args...);
//
// Don't store this function result, as it does not extends the temporaries 'args...' lifetime.
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

// std::optional.

template <class T>
struct error_traits<std::optional<T>>
{
    static constexpr bool contains_value = true;

    static constexpr T extract_value(std::optional<T>&& opt) noexcept {
        return *std::move(opt);
    }

    static constexpr bool indicates_error(std::optional<T> const& opt) noexcept {
        return !opt.has_value();
    }

    static std::string_view error_info(std::optional<T> const&) {
        return "Tried to unwrap empty optional";
    }
};

// std::error_code.

namespace detail {

    template <class...StringViews>
    std::string concatene_impl(StringViews const&...svs) {
        auto res = std::string{};
        res.reserve((0 + ... + svs.size()));
        (res.append(svs), ...);
        return res;
    }
    template <class...Strings>
    std::string concatene(Strings const&...strs) {
        return concatene_impl(std::string_view{strs}...);
    }

    bool errc_indicates_error(std::error_code const& err) noexcept {
        return static_cast<bool>(err);
    }
    inline std::string errc_info(std::error_code const& err) {
        return concatene("From error category '", err.category().name(), "' : ", err.message());
    }
}

template <>
struct error_traits<std::error_code>
{
    static constexpr bool contains_value = false;
    
    static bool indicates_error(std::error_code const& errc) noexcept {
        return detail::errc_indicates_error(errc);
    }

    static std::string error_info(std::error_code const& errc) {
        return detail::errc_info(errc);
    }
};

// Tuple<T, std::error_code>.

namespace detail {
    template <template <class...> class Tuple, class T>
    struct value_and_errc
    {
        using type = Tuple<T, std::error_code>;

        static constexpr bool contains_value = true;

        static constexpr T extract_value(type&& tuple) noexcept {
            return std::move(std::get<0>(tuple));
        }

        static constexpr bool indicates_error(type const& tuple) noexcept {
            return errc_indicates_error(std::get<1>(tuple));
        }

        static std::string error_info(type const& tuple) {
            return errc_info(std::get<1>(tuple));
        }
    };
}

// Tuple<Ts..., std::error_code>.

namespace detail {
    template <template <class...> class Tuple, class...Ts>
    struct values_and_errc
    {
        using type = Tuple<Ts..., std::error_code>;

        static constexpr bool contains_value = true;

        template <size_t...Is>
        static constexpr Tuple<Ts...> extract_value_impl(type&& tuple, std::index_sequence<Is...>) noexcept {
            return { std::move(std::get<Is>(tuple))... };
        }
        static constexpr Tuple<Ts...> extract_value(type&& tuple) noexcept {
            return extract_value_impl(std::move(tuple), std::make_index_sequence<sizeof...(Ts)>{});
        }

        static constexpr bool indicates_error(type const& tuple) noexcept {
            return errc_indicates_error(std::get<sizeof...(Ts)>(tuple));
        }

        static std::string error_info(type const& tuple) {
            return errc_info(std::get<sizeof...(Ts)>(tuple));
        }
    };
}

// std::pair and std::tuple with trailing std::error_code.

template <class T>
struct error_traits<std::pair<T, std::error_code>> :
    detail::value_and_errc<std::pair, T> {};
    
template <class T>
struct error_traits<std::tuple<T, std::error_code>> :
    detail::value_and_errc<std::tuple, T> {};
    

// The specialization can't be resolved with variadic template here.
template <class T1, class T2>
struct error_traits<std::tuple<T1, T2, std::error_code>> :
    detail::values_and_errc<std::tuple, T1, T2> {};

// :(   
template <class T1, class T2, class T3>
struct error_traits<std::tuple<T1, T2, T3, std::error_code>> :
    detail::values_and_errc<std::tuple, T1, T2, T3> {};


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
