#pragma once

/**
 * @file sequential.hpp
 * @brief All sequential containers - Linear, ordered data structures
 *
 * This header includes all sequential containers for linear
 * ordered storage and access patterns.
 */

#include "pods/sequential/array.hpp"
#include "pods/sequential/bitvec.hpp"
#include "pods/sequential/cstring.hpp"
#include "pods/sequential/deque.hpp"
#include "pods/sequential/fixed_queue.hpp"
#include "pods/sequential/flat_matrix.hpp"
#include "pods/sequential/forward_list.hpp"
#include "pods/sequential/heap.hpp"
#include "pods/sequential/indexed_heap.hpp"
#include "pods/sequential/list.hpp"
#include "pods/sequential/nvec.hpp"
#include "pods/sequential/paged_vecvec.hpp"
#include "pods/sequential/queue.hpp"
#include "pods/sequential/stack.hpp"
#include "pods/sequential/string.hpp"
#include "pods/sequential/vector.hpp"
#include "pods/sequential/vectra.hpp"
#include "pods/sequential/vecvec.hpp"

// Optional short namespace alias (enabled with -DSHORT_NAMESPACE)
#if defined(SHORT_NAMESPACE)
#endif

// Short namespace alias (disable with -DNO_SHORT_NAMESPACE)
#if !defined(NO_SHORT_NAMESPACE)
namespace dp = datapod;
#endif
