#pragma once

// Phase 1: Foundation (Core Utilities)
#include "datapod/core/decay.hpp"
#include "datapod/core/exception.hpp"
#include "datapod/core/hash.hpp"
#include "datapod/core/mode.hpp"
#include "datapod/core/offset_t.hpp"
#include "datapod/core/type_traits.hpp"
#include "datapod/core/verify.hpp"

// Phase 2: Core Utilities Extended
#include "datapod/core/aligned_alloc.hpp"
#include "datapod/core/bit_counting.hpp"
#include "datapod/core/buffer.hpp"
#include "datapod/core/chunk.hpp"
#include "datapod/core/mmap.hpp"
#include "datapod/core/next_power_of_2.hpp"
#include "datapod/core/strong.hpp"

// Phase 3: Pointer System
#include "datapod/memory/allocator.hpp"
#include "datapod/memory/offset_ptr.hpp"
#include "datapod/memory/ptr.hpp"

// Phase 4: Reflection System
#include "datapod/reflection/arity.hpp"
#include "datapod/reflection/comparable.hpp"
#include "datapod/reflection/for_each_field.hpp"
#include "datapod/reflection/to_tuple.hpp"

// Phase 5: Basic Containers
#include "datapod/adapters/optional.hpp"
#include "datapod/adapters/pair.hpp"
#include "datapod/adapters/unique_ptr.hpp"
#include "datapod/sequential/array.hpp"
#include "datapod/sequential/string.hpp"
#include "datapod/sequential/vector.hpp"

// Hashing (depends on containers and reflection)
#include "datapod/core/equal_to.hpp"
#include "datapod/hashing.hpp"

// Phase 6: Hash Containers (Swiss Tables)
#include "datapod/associative/hash_storage.hpp"
#include "datapod/associative/map.hpp"
#include "datapod/associative/set.hpp"

// Phase 7: Advanced Containers
#include "datapod/adapters/tuple.hpp"
#include "datapod/adapters/variant.hpp"

// Phase 8: Type Hash System
#include "datapod/type_hash/type_hash.hpp"
#include "datapod/type_hash/type_name.hpp"

// Phase 9: Endian Handling
#include "datapod/core/endian.hpp"

// Phase 10: Serialization Infrastructure
#include "datapod/serialization/buf.hpp"
#include "datapod/serialization/serialize.hpp"
#include "datapod/serialization/serialized_size.hpp"
