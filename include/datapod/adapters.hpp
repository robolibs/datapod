#pragma once

/**
 * @file adapters.hpp
 * @brief All adapter types - Container adapters and wrappers
 *
 * This header includes all adapter types that provide type-safe
 * wrappers and adapters for values and containers.
 */

// Core adapters
#include "adapters/bitset.hpp"
#include "adapters/error.hpp"
#include "adapters/lazy.hpp"
#include "adapters/once_cell.hpp"
#include "adapters/optional.hpp"
#include "adapters/pair.hpp"
#include "adapters/result.hpp"
#include "adapters/tuple.hpp"
#include "adapters/unique_ptr.hpp"
#include "adapters/variant.hpp"

// Conversions (must be after optional and result)
#include "adapters/conversions.hpp"

// Utility
#include "core/none.hpp"

// Optional short namespace alias (enabled with -DSHORT_NAMESPACE)
#if defined(SHORT_NAMESPACE)
namespace dp = datapod;
#endif
