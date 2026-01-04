#pragma once

#include "optional.hpp"
#include "result.hpp"

namespace datapod {

    // Optional to Result conversions
    template <typename T> template <typename E> constexpr auto Optional<T>::ok_or(E &&err) const & -> Result<T, E> {
        if (has_value_) {
            return Result<T, E>::ok(**this);
        }
        return Result<T, E>::err(std::forward<E>(err));
    }

    template <typename T> template <typename E> constexpr auto Optional<T>::ok_or(E &&err) && -> Result<T, E> {
        if (has_value_) {
            return Result<T, E>::ok(std::move(**this));
        }
        return Result<T, E>::err(std::forward<E>(err));
    }

    template <typename T>
    template <typename F>
    constexpr auto Optional<T>::ok_or_else(F &&f) const & -> Result<T, decltype(std::invoke(std::forward<F>(f)))> {
        using E = decltype(std::invoke(std::forward<F>(f)));
        if (has_value_) {
            return Result<T, E>::ok(**this);
        }
        return Result<T, E>::err(std::invoke(std::forward<F>(f)));
    }

    template <typename T>
    template <typename F>
    constexpr auto Optional<T>::ok_or_else(F &&f) && -> Result<T, decltype(std::invoke(std::forward<F>(f)))> {
        using E = decltype(std::invoke(std::forward<F>(f)));
        if (has_value_) {
            return Result<T, E>::ok(std::move(**this));
        }
        return Result<T, E>::err(std::invoke(std::forward<F>(f)));
    }

    // Optional transpose: Optional<Result<T, E>> to Result<Optional<T>, E>
    template <typename T, typename E> inline Result<Optional<T>, E> transpose(const Optional<Result<T, E>> &opt) {
        if (!opt.has_value()) {
            return Result<Optional<T>, E>::ok(Optional<T>{});
        }
        const Result<T, E> &res = *opt;
        if (res.is_ok()) {
            return Result<Optional<T>, E>::ok(Optional<T>{res.value()});
        }
        return Result<Optional<T>, E>::err(res.error());
    }

    template <typename T, typename E> inline Result<Optional<T>, E> transpose(Optional<Result<T, E>> &&opt) {
        if (!opt.has_value()) {
            return Result<Optional<T>, E>::ok(Optional<T>{});
        }
        Result<T, E> res = std::move(*opt);
        if (res.is_ok()) {
            return Result<Optional<T>, E>::ok(Optional<T>{std::move(res).value()});
        }
        return Result<Optional<T>, E>::err(std::move(res).error());
    }

    namespace conversions {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace conversions

} // namespace datapod
