#pragma once
#include <datapod/types/types.hpp>

#include <algorithm>
#include <cstddef>

namespace datapod {

    // Process data in chunks, calling fn(offset, chunk_size) for each chunk
    template <typename Fn> void chunk(unsigned const chunk_size, datapod::usize const total, Fn fn) {
        datapod::usize offset = 0U;
        datapod::usize remaining = total;
        while (remaining != 0U) {
            auto const curr_chunk_size =
                static_cast<unsigned>(std::min(remaining, static_cast<datapod::usize>(chunk_size)));
            fn(offset, curr_chunk_size);
            offset += curr_chunk_size;
            remaining -= curr_chunk_size;
        }
    }

} // namespace datapod
