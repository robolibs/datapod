#pragma once

// Phase 1: Foundation (Core Utilities)
#include "bitcon/core/decay.hpp"
#include "bitcon/core/exception.hpp"
#include "bitcon/core/hash.hpp"
#include "bitcon/core/mode.hpp"
#include "bitcon/core/offset_t.hpp"
#include "bitcon/core/type_traits.hpp"
#include "bitcon/core/verify.hpp"

// Phase 2: Core Utilities Extended
#include "bitcon/core/aligned_alloc.hpp"
#include "bitcon/core/bit_counting.hpp"
#include "bitcon/core/buffer.hpp"
#include "bitcon/core/chunk.hpp"
#include "bitcon/core/mmap.hpp"
#include "bitcon/core/next_power_of_2.hpp"
#include "bitcon/core/strong.hpp"

// Phase 3: Pointer System
#include "bitcon/containers/allocator.hpp"
#include "bitcon/containers/offset_ptr.hpp"
#include "bitcon/containers/ptr.hpp"

// Phase 4: Reflection System
#include "bitcon/reflection/arity.hpp"
#include "bitcon/reflection/comparable.hpp"
#include "bitcon/reflection/for_each_field.hpp"
#include "bitcon/reflection/to_tuple.hpp"

// Phase 5: Basic Containers
#include "bitcon/containers/array.hpp"
#include "bitcon/containers/optional.hpp"
#include "bitcon/containers/pair.hpp"
#include "bitcon/containers/string.hpp"
#include "bitcon/containers/unique_ptr.hpp"
#include "bitcon/containers/vector.hpp"

// Hashing (depends on containers and reflection)
#include "bitcon/core/equal_to.hpp"
#include "bitcon/hashing.hpp"

// Phase 6: Hash Containers (Swiss Tables)
#include "bitcon/containers/hash_map.hpp"
#include "bitcon/containers/hash_set.hpp"
#include "bitcon/containers/hash_storage.hpp"

// Phase 7: Advanced Containers
#include "bitcon/containers/tuple.hpp"
#include "bitcon/containers/variant.hpp"

// Phase 8: Type Hash System
#include "bitcon/type_hash/type_hash.hpp"
#include "bitcon/type_hash/type_name.hpp"

// Phase 9: Endian Handling
#include "bitcon/core/endian.hpp"

// Phase 10: Serialization Infrastructure
#include "bitcon/serialization/buf.hpp"
#include "bitcon/serialization/serialize.hpp"
#include "bitcon/serialization/serialized_size.hpp"
