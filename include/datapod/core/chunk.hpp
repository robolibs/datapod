#pragma once

#include <algorithm>
#include <cstddef>

namespace datapod {

    // Process data in chunks, calling fn(offset, chunk_size) for each chunk
    template <typename Fn> void chunk(unsigned const chunk_size, std::size_t const total, Fn fn) {
        std::size_t offset = 0U;
        std::size_t remaining = total;
        while (remaining != 0U) {
            auto const curr_chunk_size =
                static_cast<unsigned>(std::min(remaining, static_cast<std::size_t>(chunk_size)));
            fn(offset, curr_chunk_size);
            offset += curr_chunk_size;
            remaining -= curr_chunk_size;
        }
    }

} // namespace datapod
