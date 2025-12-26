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
 * Mathematical types (in mat::):
 *   - complex<T>          : Complex numbers (a + bi)
 *   - dual<T>             : Dual numbers for automatic differentiation
 *   - fraction<T>         : Rational numbers (numerator/denominator)
 *   - interval<T>         : Interval arithmetic [lo, hi]
 *   - polynomial<T, N>    : Fixed-degree polynomials
 *   - dual_quaternion<T>  : Rigid body transformations
 *   - phasor<T>           : AC circuit analysis (magnitude ∠ phase)
 *   - modular<T, N>       : Modular arithmetic (Z/nZ)
 *   - octonion<T>         : 8D hypercomplex numbers
 *   - bigint<N>           : Fixed-size big integers
 *
 * All types are in the datapod::mat namespace.
 */

// Core tensor types
#include "matrix/matrix.hpp"
#include "matrix/scalar.hpp"
#include "matrix/tensor.hpp"
#include "matrix/vector.hpp"

// Mathematical types
#include "matrix/math/bigint.hpp"
#include "matrix/math/complex.hpp"
#include "matrix/math/dual.hpp"
#include "matrix/math/dual_quaternion.hpp"
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
