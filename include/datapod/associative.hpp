#pragma once

/**
 * @file associative.hpp
 * @brief All associative containers - Hash-based maps and sets
 *
 * This header includes all associative containers for key-value
 * storage and fast lookup operations.
 */

#include "associative/fws_multimap.hpp"
#include "associative/map.hpp"
#include "associative/mutable_fws_multimap.hpp"
#include "associative/set.hpp"

// Optional short namespace alias (enabled with -DSHORT_NAMESPACE)
#if defined(SHORT_NAMESPACE)
namespace dp = datapod;
#endif
