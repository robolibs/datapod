#pragma once

#include "bitcon/reflection/to_tuple.hpp"

namespace bitcon {

    // CRTP mixin to auto-generate comparison operators
    template <typename Derived> struct Comparable {
        friend constexpr bool operator==(Derived const &a, Derived const &b) { return to_tuple(a) == to_tuple(b); }

        friend constexpr bool operator!=(Derived const &a, Derived const &b) { return !(a == b); }

        friend constexpr bool operator<(Derived const &a, Derived const &b) { return to_tuple(a) < to_tuple(b); }

        friend constexpr bool operator<=(Derived const &a, Derived const &b) { return to_tuple(a) <= to_tuple(b); }

        friend constexpr bool operator>(Derived const &a, Derived const &b) { return to_tuple(a) > to_tuple(b); }

        friend constexpr bool operator>=(Derived const &a, Derived const &b) { return to_tuple(a) >= to_tuple(b); }
    };

} // namespace bitcon
