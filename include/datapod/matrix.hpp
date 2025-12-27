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
#include "matrix/matrix.hpp"
#include "matrix/scalar.hpp"
#include "matrix/tensor.hpp"
#include "matrix/vector.hpp"

// Dynamic tensor types (runtime-sized)
#include "matrix/dynamic.hpp"

// Mathematical types
#include "matrix/math/bigint.hpp"
#include "matrix/math/complex.hpp"
#include "matrix/math/dual.hpp"
#include "matrix/math/fraction.hpp"
#include "matrix/math/hypercomplex.hpp"
#include "matrix/math/interval.hpp"
#include "matrix/math/modular.hpp"
#include "matrix/math/phasor.hpp"
#include "matrix/math/polynomial.hpp"

// Optional short namespace alias (enabled with -DSHORT_NAMESPACE)
#if defined(SHORT_NAMESPACE)
namespace dp = datapod;
#endif
