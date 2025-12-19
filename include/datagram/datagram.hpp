#pragma once

// Phase 1: Foundation (Core Utilities)
#include "datagram/core/decay.hpp"
#include "datagram/core/exception.hpp"
#include "datagram/core/hash.hpp"
#include "datagram/core/mode.hpp"
#include "datagram/core/offset_t.hpp"
#include "datagram/core/type_traits.hpp"
#include "datagram/core/verify.hpp"

// Phase 2: Core Utilities Extended
#include "datagram/core/aligned_alloc.hpp"
#include "datagram/core/bit_counting.hpp"
#include "datagram/core/buffer.hpp"
#include "datagram/core/chunk.hpp"
#include "datagram/core/mmap.hpp"
#include "datagram/core/next_power_of_2.hpp"
#include "datagram/core/strong.hpp"

// Phase 3: Pointer System
#include "datagram/containers/allocator.hpp"
#include "datagram/containers/offset_ptr.hpp"
#include "datagram/containers/ptr.hpp"

// Phase 4: Reflection System
#include "datagram/reflection/arity.hpp"
#include "datagram/reflection/comparable.hpp"
#include "datagram/reflection/for_each_field.hpp"
#include "datagram/reflection/to_tuple.hpp"

// Phase 5: Basic Containers
#include "datagram/containers/array.hpp"
#include "datagram/containers/optional.hpp"
#include "datagram/containers/pair.hpp"
#include "datagram/containers/string.hpp"
#include "datagram/containers/unique_ptr.hpp"
#include "datagram/containers/vector.hpp"

// Hashing (depends on containers and reflection)
#include "datagram/hashing.hpp"
