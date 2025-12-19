#pragma once

#include <tuple>
#include <utility>

#include "datagram/reflection/to_tuple.hpp"

namespace datagram {

    // Apply a function to each field of a struct
    template <typename T, typename Fn> constexpr void for_each_field(T &t, Fn &&fn) {
        auto tup = to_tuple(t);
        std::apply([&fn](auto &&...args) { (fn(std::forward<decltype(args)>(args)), ...); }, tup);
    }

    // Apply a function to each field with index
    template <typename T, typename Fn> constexpr void for_each_field_indexed(T &t, Fn &&fn) {
        auto tup = to_tuple(t);
        [&fn, &tup]<std::size_t... Is>(std::index_sequence<Is...>) {
            (fn(std::get<Is>(tup), std::integral_constant<std::size_t, Is>{}), ...);
        }(std::make_index_sequence<std::tuple_size_v<decltype(tup)>>{});
    }

    // Apply a function to each field with index
    template <typename T, typename Fn> constexpr void for_each_field_indexed(T &&t, Fn &&fn) {
        auto tup = to_tuple(std::forward<T>(t));
        [&fn, &tup]<std::size_t... Is>(std::index_sequence<Is...>) {
            (fn(std::get<Is>(tup), std::integral_constant<std::size_t, Is>{}), ...);
        }(std::make_index_sequence<std::tuple_size_v<decltype(tup)>>{});
    }

} // namespace datagram
