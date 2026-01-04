#pragma once

/**
 * @file matrix.hpp
 * @brief All matrix types - Linear algebra and tensor operations
 *
 * This header includes all matrix and tensor types for
 * multi-dimensional array operations and linear algebra.
 *
 * Fixed-size types (compile-time dimensions):
 *   - scalar<T>           : rank-0 (0D) - single value
 *   - vector<T, N>        : rank-1 (1D) - fixed-size vector
 *   - matrix<T, R, C>     : rank-2 (2D) - fixed-size matrix
 *   - tensor<T, Dims...>  : rank-N (3D+) - N-dimensional tensor
 *
 * Dynamic types (runtime dimensions, Eigen-style):
 *   - dynamic_vector<T>   : rank-1 (1D) - runtime-sized vector (VectorXd)
 *   - dynamic_matrix<T>   : rank-2 (2D) - runtime-sized matrix (MatrixXd)
 *   - dynamic_tensor<T>   : rank-N      - runtime-ranked tensor (TensorXd)
 *
 * Mathematical types (in mat::):
 *   - complex<T>          : Complex numbers (a + bi)
 *   - dual<T>             : Dual numbers for automatic differentiation
 *   - fraction<T>         : Rational numbers (numerator/denominator)
 *   - interval<T>         : Interval arithmetic [lo, hi]
 *   - polynomial<T, N>    : Fixed-degree polynomials
 *   - phasor<T>           : AC circuit analysis (magnitude ∠ phase)
 *   - modular<T, N>       : Modular arithmetic (Z/nZ)
 *   - octonion<T>         : 8D hypercomplex numbers
 *   - bigint<N>           : Fixed-size big integers
 *
 * Note: For rigid body transforms (rotation + translation), see
 *       datapod::Transform in spatial/transform.hpp
 *
 * All types are in the datapod::mat namespace.
 */

// Core tensor types (fixed-size)
#include "pods/matrix/matrix.hpp"
#include "pods/matrix/scalar.hpp"
#include "pods/matrix/tensor.hpp"
#include "pods/matrix/vector.hpp"

// Dynamic tensor types (runtime-sized)
#include "pods/matrix/dynamic.hpp"

// Mathematical types
#include "pods/matrix/math/bigint.hpp"
#include "pods/matrix/math/complex.hpp"
#include "pods/matrix/math/dual.hpp"
#include "pods/matrix/math/fraction.hpp"
#include "pods/matrix/math/hypercomplex.hpp"
#include "pods/matrix/math/interval.hpp"
#include "pods/matrix/math/modular.hpp"
#include "pods/matrix/math/phasor.hpp"
#include "pods/matrix/math/polynomial.hpp"

// Note: dp:: namespace is now used for primitive types (see types/types.hpp)
// The old SHORT_NAMESPACE alias has been removed

// Short namespace alias (disable with -DNO_SHORT_NAMESPACE)
#if !defined(NO_SHORT_NAMESPACE)
namespace dp = datapod;
#endif
