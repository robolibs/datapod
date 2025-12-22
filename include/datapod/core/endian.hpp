#pragma once

#include <cstdint>

#include "datapod/core/mode.hpp"

// Based on cista endian detection and conversion
// https://github.com/felixguendling/cista

// ============================================================================
// Endian Detection
// ============================================================================

#if !defined(DATAGRAM_BIG_ENDIAN) && !defined(DATAGRAM_LITTLE_ENDIAN)

#if defined(__APPLE__)
#include <machine/endian.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__GNUC__)
#include <endian.h>
#endif

#if defined(REG_DWORD) && REG_DWORD == REG_DWORD_BIG_ENDIAN ||                                                         \
    defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || defined(__BIG_ENDIAN__) || defined(__ARMEB__) ||          \
    defined(__THUMBEB__) || defined(__AARCH64EB__) || defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
#define DATAGRAM_BIG_ENDIAN
#elif defined(REG_DWORD) && REG_DWORD == REG_DWORD_LITTLE_ENDIAN ||                                                    \
    defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) ||    \
    defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
#define DATAGRAM_LITTLE_ENDIAN
#else
#error "architecture: unknown byte order"
#endif

#endif

// ============================================================================
// Byte Swap Intrinsics
// ============================================================================

#if defined(_MSC_VER)
#define DATAGRAM_BYTESWAP_16 _byteswap_ushort
#define DATAGRAM_BYTESWAP_32 _byteswap_ulong
#define DATAGRAM_BYTESWAP_64 _byteswap_uint64
#else
#define DATAGRAM_BYTESWAP_16 __builtin_bswap16
#define DATAGRAM_BYTESWAP_32 __builtin_bswap32
#define DATAGRAM_BYTESWAP_64 __builtin_bswap64
#endif

namespace datapod {

    // ============================================================================
    // Endian Swap
    // ============================================================================

    template <typename T> constexpr T endian_swap(T const t) noexcept {
        static_assert(sizeof(T) == 1U || sizeof(T) == 2U || sizeof(T) == 4U || sizeof(T) == 8U,
                      "endian_swap only supports 1, 2, 4, or 8 byte types");

        if constexpr (sizeof(T) == 1U) {
            return t;
        } else if constexpr (sizeof(T) == 2U) {
            union {
                T t;
                std::uint16_t i;
            } u{t};
            u.i = DATAGRAM_BYTESWAP_16(u.i);
            return u.t;
        } else if constexpr (sizeof(T) == 4U) {
            union {
                T t;
                std::uint32_t i;
            } u{t};
            u.i = DATAGRAM_BYTESWAP_32(u.i);
            return u.t;
        } else if constexpr (sizeof(T) == 8U) {
            union {
                T t;
                std::uint64_t i;
            } u{t};
            u.i = DATAGRAM_BYTESWAP_64(u.i);
            return u.t;
        }
    }

    // ============================================================================
    // Endian Conversion Check
    // ============================================================================

    template <Mode M> constexpr bool endian_conversion_necessary() noexcept {
        if constexpr ((M & Mode::SERIALIZE_BIG_ENDIAN) == Mode::SERIALIZE_BIG_ENDIAN) {
#if defined(DATAGRAM_BIG_ENDIAN)
            return false;
#else
            return true;
#endif
        } else {
#if defined(DATAGRAM_LITTLE_ENDIAN)
            return false;
#else
            return true;
#endif
        }
    }

    // ============================================================================
    // Convert Endian Based on Mode
    // ============================================================================

    template <Mode M, typename T> constexpr T convert_endian(T t) noexcept {
        if constexpr (endian_conversion_necessary<M>()) {
            return endian_swap(t);
        } else {
            return t;
        }
    }

} // namespace datapod

#undef DATAGRAM_BYTESWAP_16
#undef DATAGRAM_BYTESWAP_32
#undef DATAGRAM_BYTESWAP_64
