#pragma once

/// @file primitive_type_id.hpp
/// @brief Unique, stable type IDs for primitive types
///
/// This provides platform-independent type identification for serialization.
/// Each primitive type gets a unique ID that remains stable across compilers
/// and platforms, enabling reliable type checking during deserialization.

#include "datapod/core/hash.hpp"

namespace datapod {

    /// @brief Type trait providing unique IDs for primitive types
    /// @tparam T The type to get the ID for
    ///
    /// Specializations provide:
    /// - id: A unique hash_t value for the type (0 = not a primitive)
    /// - name: A human-readable name string
    template <typename T> struct primitive_type_id {
        static constexpr hash_t id = 0;
        static constexpr char const *name = nullptr;
    };

    // ============================================================================
    // Signed Integers
    // ============================================================================

    template <> struct primitive_type_id<signed char> {
        static constexpr hash_t id = 0x0001'0001'0000'0001ULL;
        static constexpr char const *name = "i8";
    };

    template <> struct primitive_type_id<signed short> {
        static constexpr hash_t id = 0x0001'0002'0000'0002ULL;
        static constexpr char const *name = "i16";
    };

    template <> struct primitive_type_id<signed int> {
        static constexpr hash_t id = 0x0001'0004'0000'0003ULL;
        static constexpr char const *name = "i32";
    };

    template <> struct primitive_type_id<signed long> {
        static constexpr hash_t id = 0x0001'0000'0000'0004ULL; // Size varies by platform
        static constexpr char const *name = "isize";           // Platform-dependent size
    };

    template <> struct primitive_type_id<signed long long> {
        static constexpr hash_t id = 0x0001'0008'0000'0005ULL;
        static constexpr char const *name = "i64";
    };

    // ============================================================================
    // Unsigned Integers
    // ============================================================================

    template <> struct primitive_type_id<unsigned char> {
        static constexpr hash_t id = 0x0002'0001'0000'0001ULL;
        static constexpr char const *name = "u8";
    };

    template <> struct primitive_type_id<unsigned short> {
        static constexpr hash_t id = 0x0002'0002'0000'0002ULL;
        static constexpr char const *name = "u16";
    };

    template <> struct primitive_type_id<unsigned int> {
        static constexpr hash_t id = 0x0002'0004'0000'0003ULL;
        static constexpr char const *name = "u32";
    };

    template <> struct primitive_type_id<unsigned long> {
        static constexpr hash_t id = 0x0002'0000'0000'0004ULL; // Size varies by platform
        static constexpr char const *name = "usize";           // Platform-dependent size
    };

    template <> struct primitive_type_id<unsigned long long> {
        static constexpr hash_t id = 0x0002'0008'0000'0005ULL;
        static constexpr char const *name = "u64";
    };

    // ============================================================================
    // Floating Point
    // ============================================================================

    template <> struct primitive_type_id<float> {
        static constexpr hash_t id = 0x0003'0004'0000'0001ULL;
        static constexpr char const *name = "f32";
    };

    template <> struct primitive_type_id<double> {
        static constexpr hash_t id = 0x0003'0008'0000'0002ULL;
        static constexpr char const *name = "f64";
    };

#ifdef __FLT16_MANT_DIG__
    template <> struct primitive_type_id<_Float16> {
        static constexpr hash_t id = 0x0003'0002'0000'0003ULL;
        static constexpr char const *name = "f16";
    };
#endif

    // ============================================================================
    // Character Types
    // ============================================================================

    // Note: char is distinct from signed char and unsigned char in C++
    template <> struct primitive_type_id<char> {
        static constexpr hash_t id = 0x0004'0001'0000'0001ULL;
        static constexpr char const *name = "char8";
    };

    template <> struct primitive_type_id<char16_t> {
        static constexpr hash_t id = 0x0004'0002'0000'0002ULL;
        static constexpr char const *name = "char16";
    };

    template <> struct primitive_type_id<char32_t> {
        static constexpr hash_t id = 0x0004'0004'0000'0003ULL;
        static constexpr char const *name = "char32";
    };

    // ============================================================================
    // Boolean
    // ============================================================================

    template <> struct primitive_type_id<bool> {
        static constexpr hash_t id = 0x0005'0001'0000'0001ULL;
        static constexpr char const *name = "boolean";
    };

    // ============================================================================
    // Helper trait
    // ============================================================================

    /// @brief Check if a type has a primitive type ID
    template <typename T> inline constexpr bool has_primitive_type_id_v = primitive_type_id<T>::id != 0;

} // namespace datapod
