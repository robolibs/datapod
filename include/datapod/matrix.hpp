#pragma once

/**
 * @file matrix.hpp
 * @brief All matrix types - Linear algebra and tensor operations
 *
 * This header includes all matrix and tensor types for
 * multi-dimensional array operations and linear algebra.
 *
 * Hierarchy:
 *   - scalar<T>           : rank-0 (0D) - single value
 *   - vector<T, N>        : rank-1 (1D) - fixed-size vector
 *   - matrix<T, R, C>     : rank-2 (2D) - fixed-size matrix
 *   - tensor<T, Dims...>  : rank-N (3D+) - N-dimensional tensor
 *
 * All types are in the datapod::mat namespace.
 */

#include "matrix/matrix.hpp"
#include "matrix/scalar.hpp"
#include "matrix/tensor.hpp"
#include "matrix/vector.hpp"

// Optional short namespace alias (enabled with -DSHORT_NAMESPACE)
#if defined(SHORT_NAMESPACE)
namespace dp = datapod;
#endif
