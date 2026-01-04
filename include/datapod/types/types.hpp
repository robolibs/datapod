#pragma once

/// @file types.hpp
/// @brief Datapod type system - Rust-inspired type aliases
///
/// This header provides convenient type aliases for fundamental types:
/// - Integer types: i8, i16, i32, i64, u8, u16, u32, u64
/// - Size types: usize, isize
/// - Floating point: f32, f64, (f16 if supported)
/// - Character types: char8, char16, char32
/// - Other: boolean, byte
///
/// Available in both `datapod::` and `dp::` namespaces
///
/// @example
/// ```cpp
/// #include <datapod/types/types.hpp>
///
/// dp::u32 count = 42;
/// dp::f64 value = 3.14159;
/// dp::usize size = sizeof(int);
/// ```

#include "primitives.hpp"

// Short namespace alias (disable with -DNO_SHORT_NAMESPACE)
#if !defined(NO_SHORT_NAMESPACE)
namespace dp = datapod;
#endif
