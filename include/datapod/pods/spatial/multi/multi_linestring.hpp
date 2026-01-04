#pragma once

#include <tuple>

#include "../linestring.hpp"
#include "datapod/pods/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief Collection of multiple linestrings (POD)
     *
     * MultiLinestring represents a collection of multiple path segments
     * in space. Each linestring is independent and may or may not connect
     * to others.
     *
     * Commonly used for representing road networks, river systems,
     * or multiple independent paths.
     *
     * Features:
     * - POD-compatible
     * - Serializable via members()
     * - Use datapod::Vector for serializability
     *
     * Note: Uses datapod::Vector instead of std::vector for serializability.
     */
    struct MultiLinestring {
        Vector<Linestring> linestrings;

        auto members() noexcept { return std::tie(linestrings); }
        auto members() const noexcept { return std::tie(linestrings); }

        inline size_t size() const noexcept { return linestrings.size(); }
        inline bool empty() const noexcept { return linestrings.empty(); }
    };

    namespace multi_linestring {
        /// Placeholder for container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace multi_linestring

} // namespace datapod
