#pragma once

#include <cstddef>

#include "datapod/core/decay.hpp"

namespace datapod {

    // Returns the serialized size of a type
    // This is simply sizeof(decay_t<T>) for most types
    template <typename T> constexpr std::size_t serialized_size(void *const param = nullptr) noexcept {
        static_cast<void>(param);
        return sizeof(decay_t<T>);
    }

} // namespace datapod
