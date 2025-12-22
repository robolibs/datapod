#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace bitcon {

    // Universal type that can convert to anything (for aggregate initialization testing)
    namespace detail {
        struct any_type {
            std::size_t ignore;
            template <typename T> constexpr operator T &() const;
            template <typename T> constexpr operator T &&() const;
        };

        // Check if T can be aggregate-initialized with N arguments
        template <typename T, typename... Args>
        constexpr auto is_n_constructible(int) -> decltype(T{std::declval<Args>()...}, std::true_type{}) {
            return {};
        }

        template <typename T, typename...> constexpr std::false_type is_n_constructible(...) { return {}; }

        template <typename T, typename... Args>
        inline constexpr bool is_n_constructible_v = decltype(is_n_constructible<T, Args...>(0))::value;

    } // namespace detail

    // Detect arity by testing aggregate initialization with increasing number of any_type arguments
    template <typename T, std::size_t N = 0, typename = void> struct ArityImpl {
        static constexpr std::size_t value = 0;
    };

    template <typename T, std::size_t N>
    struct ArityImpl<T, N, std::enable_if_t<std::is_aggregate_v<T> && !std::is_array_v<T>>> {
      private:
        template <std::size_t... Is>
        static constexpr auto test(std::index_sequence<Is...>)
            -> decltype(T{(Is, detail::any_type{})...}, std::true_type{}) {
            return {};
        }

        static constexpr std::false_type test(...) { return {}; }

        static constexpr bool can_construct_with_n = decltype(test(std::make_index_sequence<N>{}))::value;
        static constexpr bool can_construct_with_n_plus_1 = decltype(test(std::make_index_sequence<N + 1>{}))::value;

      public:
        static constexpr std::size_t value =
            can_construct_with_n && !can_construct_with_n_plus_1 ? N : ArityImpl<T, N + 1>::value;
    };

    // Termination specialization
    template <typename T> struct ArityImpl<T, 128, std::enable_if_t<std::is_aggregate_v<T> && !std::is_array_v<T>>> {
        static constexpr std::size_t value = 128; // Max supported arity
    };

    template <typename T> struct Arity {
        static constexpr std::size_t value = ArityImpl<T>::value;
    };

    template <typename T> inline constexpr std::size_t arity_v = Arity<T>::value;

    // Check if a type is reflectable (has determinable arity)
    template <typename T>
    inline constexpr bool is_reflectable_v = std::is_aggregate_v<T> && !std::is_array_v<T> && arity_v<T> > 0;

} // namespace bitcon
