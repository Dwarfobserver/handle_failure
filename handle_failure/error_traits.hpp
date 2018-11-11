
// Implements hf::error_traits for a few types which can represent an error :
//   - std::optional<T> => T
//   - std::pair<T, std::error_code> => T
//   - std::tuple<T, std::error_code> => T
//   - std::tuple<T, Ts..., std::error_code> => std::tuple<T, Ts...>

#pragma once

#include <handle_failure/core.hpp>
#include <optional>
#include <system_error>
#include <string>
#include <string_view>

namespace hf {

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

} // ::hf
