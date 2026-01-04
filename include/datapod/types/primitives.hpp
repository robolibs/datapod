#pragma once

/// @file primitives.hpp
/// @brief Fundamental type definitions without std:: dependencies
///
/// This header defines all primitive types using compiler built-ins,
/// completely independent of the standard library.

namespace datapod {

    // ============================================================================
    // Integer Types - Using compiler built-ins
    // ============================================================================

    // Signed integers
    using i8 = signed char;       // Always 8 bits
    using i16 = signed short;     // Always 16 bits
    using i32 = signed int;       // Always 32 bits
    using i64 = signed long long; // Always 64 bits

    // Unsigned integers
    using u8 = unsigned char;       // Always 8 bits
    using u16 = unsigned short;     // Always 16 bits
    using u32 = unsigned int;       // Always 32 bits
    using u64 = unsigned long long; // Always 64 bits

// Size types - platform dependent
// Note: We use the same underlying types as std::size_t/std::ptrdiff_t
// to ensure compatibility with standard library functions
#if defined(__LP64__) || defined(_WIN64)
    // 64-bit platforms
    using usize = unsigned long; // Same as std::size_t on 64-bit
    using isize = signed long;   // Same as std::ptrdiff_t on 64-bit
#else
    // 32-bit platforms
    using usize = unsigned int; // Same as std::size_t on 32-bit
    using isize = signed int;   // Same as std::ptrdiff_t on 32-bit
#endif

    // ============================================================================
    // Floating Point Types
    // ============================================================================

    using f32 = float;  // IEEE 754 single precision (32 bits)
    using f64 = double; // IEEE 754 double precision (64 bits)

// Half precision (16 bits) - compiler/hardware dependent
#ifdef __FLT16_MANT_DIG__
    using f16 = _Float16;
#endif

    // ============================================================================
    // Character Types
    // ============================================================================

    using char8 = char;      // 8-bit character
    using char16 = char16_t; // 16-bit character (UTF-16)
    using char32 = char32_t; // 32-bit character (UTF-32)

    // ============================================================================
    // Boolean Type
    // ============================================================================

    using boolean = bool; // Boolean type

    // ============================================================================
    // Byte Type
    // ============================================================================

    using byte = unsigned char; // Raw byte (8 bits)

    // ============================================================================
    // Pointer Types
    // ============================================================================

    using nullptr_t = decltype(nullptr);

    // ============================================================================
    // Compile-time size verification
    // ============================================================================

    static_assert(sizeof(i8) == 1, "i8 must be 1 byte");
    static_assert(sizeof(i16) == 2, "i16 must be 2 bytes");
    static_assert(sizeof(i32) == 4, "i32 must be 4 bytes");
    static_assert(sizeof(i64) == 8, "i64 must be 8 bytes");

    static_assert(sizeof(u8) == 1, "u8 must be 1 byte");
    static_assert(sizeof(u16) == 2, "u16 must be 2 bytes");
    static_assert(sizeof(u32) == 4, "u32 must be 4 bytes");
    static_assert(sizeof(u64) == 8, "u64 must be 8 bytes");

    static_assert(sizeof(f32) == 4, "f32 must be 4 bytes");
    static_assert(sizeof(f64) == 8, "f64 must be 8 bytes");

    static_assert(sizeof(byte) == 1, "byte must be 1 byte");

} // namespace datapod
