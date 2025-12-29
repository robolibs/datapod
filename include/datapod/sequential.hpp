#pragma once

/**
 * @file sequential.hpp
 * @brief All sequential containers - Linear, ordered data structures
 *
 * This header includes all sequential containers for linear
 * ordered storage and access patterns.
 */

#include "sequential/array.hpp"
#include "sequential/bitvec.hpp"
#include "sequential/cstring.hpp"
#include "sequential/deque.hpp"
#include "sequential/fixed_queue.hpp"
#include "sequential/flat_matrix.hpp"
#include "sequential/forward_list.hpp"
#include "sequential/heap.hpp"
#include "sequential/indexed_heap.hpp"
#include "sequential/list.hpp"
#include "sequential/nvec.hpp"
#include "sequential/paged_vecvec.hpp"
#include "sequential/queue.hpp"
#include "sequential/stack.hpp"
#include "sequential/string.hpp"
#include "sequential/vector.hpp"
#include "sequential/vecvec.hpp"

// Optional short namespace alias (enabled with -DSHORT_NAMESPACE)
#if defined(SHORT_NAMESPACE)
namespace dp = datapod;
#endif
