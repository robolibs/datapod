#pragma once

#include "datapod/pods/adapters/variant.hpp"
#include <tuple>
#include <utility>

namespace datapod {

    /// Either<L, R> - Represents a value that can be one of two types
    /// Similar to Haskell's Either and Scala's Either
    /// More semantic than Variant for binary choices
    /// By convention: Left is often used for "failure" or "alternative" path
    ///                Right is often used for "success" or "primary" path
    template <typename L, typename R> struct Either {
        Variant<L, R> data;

        auto members() noexcept { return std::tie(data); }
        auto members() const noexcept { return std::tie(data); }

        // Construction
        Either() = default;

        /// Create a Left value
        static Either left(L const &value) {
            Either e;
            e.data.template emplace<0>(value);
            return e;
        }

        static Either left(L &&value) {
            Either e;
            e.data.template emplace<0>(std::move(value));
            return e;
        }

        /// Create a Right value
        static Either right(R const &value) {
            Either e;
            e.data.template emplace<1>(value);
            return e;
        }

        static Either right(R &&value) {
            Either e;
            e.data.template emplace<1>(std::move(value));
            return e;
        }

        // Queries
        bool is_left() const noexcept { return data.index() == 0; }
        bool is_right() const noexcept { return data.index() == 1; }

        // Access - throws if wrong variant
        L &left_value() & { return get<L>(data); }
        L const &left_value() const & { return get<L>(data); }
        L &&left_value() && { return std::move(get<L>(data)); }

        R &right_value() & { return get<R>(data); }
        R const &right_value() const & { return get<R>(data); }
        R &&right_value() && { return std::move(get<R>(data)); }

        // Monadic operations

        /// Map over the Right value
        template <typename F> auto map_right(F &&f) const & {
            using U = decltype(f(right_value()));
            if (is_right()) {
                return Either<L, U>::right(f(right_value()));
            }
            return Either<L, U>::left(left_value());
        }

        template <typename F> auto map_right(F &&f) && {
            using U = decltype(f(std::move(*this).right_value()));
            if (is_right()) {
                return Either<L, U>::right(f(std::move(*this).right_value()));
            }
            return Either<L, U>::left(std::move(*this).left_value());
        }

        /// Map over the Left value
        template <typename F> auto map_left(F &&f) const & {
            using U = decltype(f(left_value()));
            if (is_left()) {
                return Either<U, R>::left(f(left_value()));
            }
            return Either<U, R>::right(right_value());
        }

        template <typename F> auto map_left(F &&f) && {
            using U = decltype(f(std::move(*this).left_value()));
            if (is_left()) {
                return Either<U, R>::left(f(std::move(*this).left_value()));
            }
            return Either<U, R>::right(std::move(*this).right_value());
        }

        /// Map over both sides
        template <typename FL, typename FR> auto bimap(FL &&fl, FR &&fr) const & {
            using UL = decltype(fl(left_value()));
            using UR = decltype(fr(right_value()));
            if (is_left()) {
                return Either<UL, UR>::left(fl(left_value()));
            }
            return Either<UL, UR>::right(fr(right_value()));
        }

        template <typename FL, typename FR> auto bimap(FL &&fl, FR &&fr) && {
            using UL = decltype(fl(std::move(*this).left_value()));
            using UR = decltype(fr(std::move(*this).right_value()));
            if (is_left()) {
                return Either<UL, UR>::left(fl(std::move(*this).left_value()));
            }
            return Either<UL, UR>::right(fr(std::move(*this).right_value()));
        }

        /// Fold/match - apply one of two functions based on which side is present
        template <typename FL, typename FR> auto fold(FL &&fl, FR &&fr) const & {
            if (is_left()) {
                return fl(left_value());
            }
            return fr(right_value());
        }

        template <typename FL, typename FR> auto fold(FL &&fl, FR &&fr) && {
            if (is_left()) {
                return fl(std::move(*this).left_value());
            }
            return fr(std::move(*this).right_value());
        }

        /// Swap Left and Right
        Either<R, L> swap() const & {
            if (is_left()) {
                return Either<R, L>::right(left_value());
            }
            return Either<R, L>::left(right_value());
        }

        Either<R, L> swap() && {
            if (is_left()) {
                return Either<R, L>::right(std::move(*this).left_value());
            }
            return Either<R, L>::left(std::move(*this).right_value());
        }

        /// Get the Right value or a default
        template <typename U> R right_or(U &&default_value) const & {
            if (is_right()) {
                return right_value();
            }
            return std::forward<U>(default_value);
        }

        template <typename U> R right_or(U &&default_value) && {
            if (is_right()) {
                return std::move(*this).right_value();
            }
            return std::forward<U>(default_value);
        }

        /// Get the Left value or a default
        template <typename U> L left_or(U &&default_value) const & {
            if (is_left()) {
                return left_value();
            }
            return std::forward<U>(default_value);
        }

        template <typename U> L left_or(U &&default_value) && {
            if (is_left()) {
                return std::move(*this).left_value();
            }
            return std::forward<U>(default_value);
        }

        /// Inspect the Right value without consuming
        template <typename F> Either const &inspect_right(F &&f) const & {
            if (is_right()) {
                f(right_value());
            }
            return *this;
        }

        template <typename F> Either &inspect_right(F &&f) & {
            if (is_right()) {
                f(right_value());
            }
            return *this;
        }

        /// Inspect the Left value without consuming
        template <typename F> Either const &inspect_left(F &&f) const & {
            if (is_left()) {
                f(left_value());
            }
            return *this;
        }

        template <typename F> Either &inspect_left(F &&f) & {
            if (is_left()) {
                f(left_value());
            }
            return *this;
        }

        // Comparison operators
        friend bool operator==(Either const &a, Either const &b) { return a.data == b.data; }
        friend bool operator!=(Either const &a, Either const &b) { return a.data != b.data; }
        friend bool operator<(Either const &a, Either const &b) { return a.data < b.data; }
        friend bool operator<=(Either const &a, Either const &b) { return a.data <= b.data; }
        friend bool operator>(Either const &a, Either const &b) { return a.data > b.data; }
        friend bool operator>=(Either const &a, Either const &b) { return a.data >= b.data; }
    };

    /// Helper functions for creating Either values
    template <typename L, typename R> Either<L, R> Left(L value) { return Either<L, R>::left(std::move(value)); }

    template <typename L, typename R> Either<L, R> Right(R value) { return Either<L, R>::right(std::move(value)); }

    namespace either {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace either

} // namespace datapod
