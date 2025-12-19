#pragma once

#include <functional>
#include <type_traits>

namespace datagram {

    namespace detail {

        // Primary template: standard decay behavior
        template <typename T> struct Decay {
            using type = std::remove_cv_t<std::remove_reference_t<T>>;
        };

        // Specialization for reference_wrapper: unwrap the reference
        template <typename T> struct Decay<std::reference_wrapper<T>> {
            using type = std::remove_cv_t<std::remove_reference_t<T>>;
        };

    } // namespace detail

    // Custom decay that also handles std::reference_wrapper
    template <typename T> using decay_t = typename detail::Decay<std::remove_reference_t<T>>::type;

} // namespace datagram
