#pragma once
#include <datapod/types/types.hpp>

#include <tuple>
#include <type_traits>

#include "datapod/reflection/arity.hpp"
#include "datapod/reflection/has_members.hpp"

namespace datapod {

    // Convert aggregate types to tuples using structured bindings
    namespace detail {
        template <typename T, datapod::usize N> struct ToTupleImpl;

        // Specializations for 0-10 fields
        template <typename T> struct ToTupleImpl<T, 0> {
            static constexpr auto to_tuple(T &) { return std::tuple<>{}; }
        };

        template <typename T> struct ToTupleImpl<T, 1> {
            static constexpr auto to_tuple(T &t) {
                auto &[a] = t;
                return std::tie(a);
            }
        };

        template <typename T> struct ToTupleImpl<T, 2> {
            static constexpr auto to_tuple(T &t) {
                auto &[a, b] = t;
                return std::tie(a, b);
            }
        };

        template <typename T> struct ToTupleImpl<T, 3> {
            static constexpr auto to_tuple(T &t) {
                auto &[a, b, c] = t;
                return std::tie(a, b, c);
            }
        };

        template <typename T> struct ToTupleImpl<T, 4> {
            static constexpr auto to_tuple(T &t) {
                auto &[a, b, c, d] = t;
                return std::tie(a, b, c, d);
            }
        };

        template <typename T> struct ToTupleImpl<T, 5> {
            static constexpr auto to_tuple(T &t) {
                auto &[a, b, c, d, e] = t;
                return std::tie(a, b, c, d, e);
            }
        };

        template <typename T> struct ToTupleImpl<T, 6> {
            static constexpr auto to_tuple(T &t) {
                auto &[a, b, c, d, e, f] = t;
                return std::tie(a, b, c, d, e, f);
            }
        };

        template <typename T> struct ToTupleImpl<T, 7> {
            static constexpr auto to_tuple(T &t) {
                auto &[a, b, c, d, e, f, g] = t;
                return std::tie(a, b, c, d, e, f, g);
            }
        };

        template <typename T> struct ToTupleImpl<T, 8> {
            static constexpr auto to_tuple(T &t) {
                auto &[a, b, c, d, e, f, g, h] = t;
                return std::tie(a, b, c, d, e, f, g, h);
            }
        };

        template <typename T> struct ToTupleImpl<T, 9> {
            static constexpr auto to_tuple(T &t) {
                auto &[a, b, c, d, e, f, g, h, i] = t;
                return std::tie(a, b, c, d, e, f, g, h, i);
            }
        };

        template <typename T> struct ToTupleImpl<T, 10> {
            static constexpr auto to_tuple(T &t) {
                auto &[a, b, c, d, e, f, g, h, i, j] = t;
                return std::tie(a, b, c, d, e, f, g, h, i, j);
            }
        };

    } // namespace detail

} // namespace datapod

// ============================================================================
// EXTENDED REFLECTION: 11-64 fields (auto-generated)
// ============================================================================
// Datapod now supports automatic reflection for structs with up to 64 fields!
// No need to manually write members() for most structs.
// The arity detection (arity.hpp) supports up to 128 fields.
#include "to_tuple_extended.hpp"

namespace datapod {

    // ============================================================================
    // ENHANCED to_tuple - checks for members() first!
    // ============================================================================

    // Main to_tuple function
    template <typename T> constexpr auto to_tuple(T &t) {
        using Type = std::remove_cv_t<std::remove_reference_t<T>>;

        // Priority 1: Check for members() function
        if constexpr (has_members_v<Type>) {
            return t.members();
        }
        // Priority 2: Fall back to automatic structured bindings
        else {
            return detail::ToTupleImpl<Type, arity_v<Type>>::to_tuple(t);
        }
    }

    // Const overload
    template <typename T> constexpr auto to_tuple(T const &t) {
        using Type = std::remove_cv_t<std::remove_reference_t<T>>;

        // Priority 1: Check for const members() function
        if constexpr (has_const_members_v<Type>) {
            return t.members();
        }
        // Priority 2: Fall back to automatic structured bindings
        else {
            return detail::ToTupleImpl<Type, arity_v<Type>>::to_tuple(const_cast<T &>(t));
        }
    }

} // namespace datapod
