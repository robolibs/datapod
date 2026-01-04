#pragma once

/**
 * @file adapters.hpp
 * @brief All adapter types - Container adapters and wrappers
 *
 * This header includes all adapter types that provide type-safe
 * wrappers and adapters for values and containers.
 */

// Core adapters
#include "pods/adapters/bitset.hpp"
#include "pods/adapters/cow.hpp"
#include "pods/adapters/either.hpp"
#include "pods/adapters/error.hpp"
#include "pods/adapters/lazy.hpp"
#include "pods/adapters/maybe_uninit.hpp"
#include "pods/adapters/non_null.hpp"
#include "pods/adapters/once_cell.hpp"
#include "pods/adapters/optional.hpp"
#include "pods/adapters/pair.hpp"
#include "pods/adapters/pin.hpp"
#include "pods/adapters/ref_cell.hpp"
#include "pods/adapters/result.hpp"
#include "pods/adapters/shared_ptr.hpp"
#include "pods/adapters/tuple.hpp"
#include "pods/adapters/unique_ptr.hpp"
#include "pods/adapters/variant.hpp"

// Conversions (must be after optional and result)
#include "pods/adapters/conversions.hpp"

// Utility
#include "core/none.hpp"

// Optional short namespace alias (enabled with -DSHORT_NAMESPACE)
#if defined(SHORT_NAMESPACE)
#endif
