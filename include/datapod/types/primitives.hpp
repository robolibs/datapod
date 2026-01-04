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
#if defined(__LP64__) || defined(_WIN64)
                                    // 64-bit platforms
    using usize = unsigned long long;
    using isize = signed long long;
#else
                                    // 32-bit platforms
    using usize = unsigned int;
    using isize = signed int;
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

// ============================================================================
// Short namespace alias: dp
// ============================================================================

namespace dp {
    // Integer types
    using datapod::i16;
    using datapod::i32;
    using datapod::i64;
    using datapod::i8;

    using datapod::u16;
    using datapod::u32;
    using datapod::u64;
    using datapod::u8;

    using datapod::isize;
    using datapod::usize;

    // Floating point types
    using datapod::f32;
    using datapod::f64;

#ifdef __FLT16_MANT_DIG__
    using datapod::f16;
#endif

    // Character types
    using datapod::char16;
    using datapod::char32;
    using datapod::char8;

    // Boolean
    using datapod::boolean;

    // Byte
    using datapod::byte;

    // Pointer
    using datapod::nullptr_t;

} // namespace dp
