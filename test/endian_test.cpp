#include <doctest/doctest.h>

#include "datagram/datagram.hpp"

using namespace datagram;

// Test endian detection
TEST_CASE("endian - detection") {
#if defined(DATAGRAM_BIG_ENDIAN)
    CHECK(true); // Big endian system
#elif defined(DATAGRAM_LITTLE_ENDIAN)
    CHECK(true); // Little endian system (most common)
#else
    FAIL("No endian detected");
#endif
}

// Test endian swap for different sizes
TEST_CASE("endian_swap - 1 byte") {
    std::uint8_t val = 0x42;
    auto swapped = endian_swap(val);
    CHECK(swapped == val); // 1 byte swap is identity
}

TEST_CASE("endian_swap - 2 bytes") {
    std::uint16_t val = 0x1234;
    auto swapped = endian_swap(val);
    CHECK(swapped == 0x3412);

    // Double swap should return original
    auto double_swapped = endian_swap(swapped);
    CHECK(double_swapped == val);
}

TEST_CASE("endian_swap - 4 bytes") {
    std::uint32_t val = 0x12345678;
    auto swapped = endian_swap(val);
    CHECK(swapped == 0x78563412);

    // Double swap should return original
    auto double_swapped = endian_swap(swapped);
    CHECK(double_swapped == val);
}

TEST_CASE("endian_swap - 8 bytes") {
    std::uint64_t val = 0x123456789ABCDEF0ULL;
    auto swapped = endian_swap(val);
    CHECK(swapped == 0xF0DEBC9A78563412ULL);

    // Double swap should return original
    auto double_swapped = endian_swap(swapped);
    CHECK(double_swapped == val);
}

// Test endian swap with signed types
TEST_CASE("endian_swap - signed types") {
    int16_t val16 = 0x1234;
    auto swapped16 = endian_swap(val16);
    CHECK(swapped16 == 0x3412);

    int32_t val32 = 0x12345678;
    auto swapped32 = endian_swap(val32);
    CHECK(swapped32 == 0x78563412);

    int64_t val64 = 0x123456789ABCDEF0LL;
    auto swapped64 = endian_swap(val64);
    CHECK(swapped64 == static_cast<int64_t>(0xF0DEBC9A78563412ULL));
}

// Test endian swap with floating point
TEST_CASE("endian_swap - float") {
    float val = 1.0f;
    auto swapped = endian_swap(val);
    auto double_swapped = endian_swap(swapped);

    // Double swap should return original value
    CHECK(double_swapped == val);
}

TEST_CASE("endian_swap - double") {
    double val = 1.0;
    auto swapped = endian_swap(val);
    auto double_swapped = endian_swap(swapped);

    // Double swap should return original value
    CHECK(double_swapped == val);
}

// Test endian conversion with different modes
TEST_CASE("endian_conversion_necessary - little endian mode") {
    constexpr auto needs_conversion = endian_conversion_necessary<Mode::NONE>();

#if defined(DATAGRAM_LITTLE_ENDIAN)
    // On little endian system with little endian mode, no conversion needed
    CHECK_FALSE(needs_conversion);
#else
    // On big endian system with little endian mode, conversion needed
    CHECK(needs_conversion);
#endif
}

TEST_CASE("endian_conversion_necessary - big endian mode") {
    constexpr auto needs_conversion = endian_conversion_necessary<Mode::SERIALIZE_BIG_ENDIAN>();

#if defined(DATAGRAM_BIG_ENDIAN)
    // On big endian system with big endian mode, no conversion needed
    CHECK_FALSE(needs_conversion);
#else
    // On little endian system with big endian mode, conversion needed
    CHECK(needs_conversion);
#endif
}

// Test convert_endian function
TEST_CASE("convert_endian - little endian mode") {
    std::uint32_t val = 0x12345678;
    auto converted = convert_endian<Mode::NONE>(val);

#if defined(DATAGRAM_LITTLE_ENDIAN)
    // No conversion on little endian system
    CHECK(converted == val);
#else
    // Conversion on big endian system
    CHECK(converted == 0x78563412);
#endif
}

TEST_CASE("convert_endian - big endian mode") {
    std::uint32_t val = 0x12345678;
    auto converted = convert_endian<Mode::SERIALIZE_BIG_ENDIAN>(val);

#if defined(DATAGRAM_BIG_ENDIAN)
    // No conversion on big endian system
    CHECK(converted == val);
#else
    // Conversion on little endian system
    CHECK(converted == 0x78563412);
#endif
}

// Test with various integer sizes
TEST_CASE("convert_endian - various sizes") {
    std::uint8_t val8 = 0x42;
    std::uint16_t val16 = 0x1234;
    std::uint32_t val32 = 0x12345678;
    std::uint64_t val64 = 0x123456789ABCDEF0ULL;

    // Convert with little endian mode
    auto conv8 = convert_endian<Mode::NONE>(val8);
    auto conv16 = convert_endian<Mode::NONE>(val16);
    auto conv32 = convert_endian<Mode::NONE>(val32);
    auto conv64 = convert_endian<Mode::NONE>(val64);

#if defined(DATAGRAM_LITTLE_ENDIAN)
    CHECK(conv8 == val8);
    CHECK(conv16 == val16);
    CHECK(conv32 == val32);
    CHECK(conv64 == val64);
#else
    CHECK(conv8 == val8); // 1 byte is always the same
    CHECK(conv16 == 0x3412);
    CHECK(conv32 == 0x78563412);
    CHECK(conv64 == 0xF0DEBC9A78563412ULL);
#endif
}

// Test round-trip conversion
TEST_CASE("convert_endian - round trip") {
    std::uint32_t original = 0x12345678;

    // Convert to big endian and back
    auto to_big = convert_endian<Mode::SERIALIZE_BIG_ENDIAN>(original);
    auto from_big = convert_endian<Mode::SERIALIZE_BIG_ENDIAN>(to_big);
    CHECK(from_big == original);

    // Convert to little endian (default) and back
    auto to_little = convert_endian<Mode::NONE>(original);
    auto from_little = convert_endian<Mode::NONE>(to_little);
    CHECK(from_little == original);
}

// Test with Mode combinations
TEST_CASE("endian_conversion_necessary - mode combinations") {
    // NONE mode (default is little endian)
    constexpr auto none_mode = endian_conversion_necessary<Mode::NONE>();

    // WITH_VERSION mode (still little endian by default)
    constexpr auto version_mode = endian_conversion_necessary<Mode::WITH_VERSION>();

    // WITH_INTEGRITY mode
    constexpr auto integrity_mode = endian_conversion_necessary<Mode::WITH_INTEGRITY>();

    // BIG_ENDIAN mode
    constexpr auto big_endian_mode = endian_conversion_necessary<Mode::SERIALIZE_BIG_ENDIAN>();

    // Combined modes
    constexpr auto combined_mode = endian_conversion_necessary<Mode::WITH_VERSION | Mode::SERIALIZE_BIG_ENDIAN>();

#if defined(DATAGRAM_LITTLE_ENDIAN)
    CHECK_FALSE(none_mode);
    CHECK_FALSE(version_mode);
    CHECK_FALSE(integrity_mode);
    CHECK(big_endian_mode);
    CHECK(combined_mode);
#else
    CHECK(none_mode);
    CHECK(version_mode);
    CHECK(integrity_mode);
    CHECK_FALSE(big_endian_mode);
    CHECK_FALSE(combined_mode);
#endif
}

// Test constexpr evaluation
TEST_CASE("endian - constexpr evaluation") {
    // Conversion necessity is constexpr
    constexpr bool needs_conv = endian_conversion_necessary<Mode::SERIALIZE_BIG_ENDIAN>();
    CHECK((needs_conv || !needs_conv)); // Always true, just testing it compiles

    // Note: endian_swap cannot be constexpr because it uses union type punning
    // which is not allowed in constant expressions
}
