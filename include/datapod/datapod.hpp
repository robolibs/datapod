#pragma once

/**
 * @file datapod.hpp
 * @brief Main header file for the Datapod library - includes EVERYTHING
 *
 * This is the main convenience header that includes all datapod types.
 * Just #include <datapod/datapod.hpp> and you're ready to go!
 *
 * For selective inclusion (to avoid potential naming conflicts), use category headers:
 * - #include <datapod/sequential.hpp>  - Vector, String, Array, Queue, Stack, etc.
 * - #include <datapod/associative.hpp> - Map, Set, multimaps
 * - #include <datapod/adapters.hpp>    - Optional, Result, Error, Variant, Pair, Tuple
 * - #include <datapod/spatial.hpp>     - Point, Pose, geometry types
 * - #include <datapod/temporal.hpp>    - TimeSeries, Stamp, Event, etc.
 * - #include <datapod/matrix.hpp>      - Matrix, Tensor, Scalar
 */

// Core utilities (always needed)
#include "core/decay.hpp"
#include "core/exception.hpp"
#include "core/hash.hpp"
#include "core/mode.hpp"
#include "core/none.hpp"
#include "core/offset_t.hpp"
#include "core/type_traits.hpp"
#include "core/verify.hpp"

// Core utilities extended
#include "core/aligned_alloc.hpp"
#include "core/bit_counting.hpp"
#include "core/buffer.hpp"
#include "core/chunk.hpp"
#include "core/endian.hpp"
#include "core/equal_to.hpp"
#include "core/next_power_of_2.hpp"
#include "core/strong.hpp"

// Memory management
#include "memory/allocator.hpp"
#include "memory/offset_ptr.hpp"
#include "memory/ptr.hpp"

// Reflection system
#include "reflection/arity.hpp"
#include "reflection/comparable.hpp"
#include "reflection/for_each_field.hpp"
#include "reflection/has_members.hpp"
#include "reflection/to_tuple.hpp"

// Hashing
#include "hashing.hpp"

// Type hash system
#include "type_hash/type_hash.hpp"
#include "type_hash/type_name.hpp"

// Serialization system
#include "serialization/buf.hpp"
#include "serialization/serialize.hpp"
#include "serialization/serialized_size.hpp"

// All category headers (everything!)
#include "adapters.hpp"
#include "associative.hpp"
#include "matrix.hpp"
#include "sequential.hpp"
#include "spatial.hpp"
#include "temporal.hpp"
