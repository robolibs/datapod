#include <doctest/doctest.h>

#include "datapod/sequential/bitvec.hpp"

using namespace datapod;

TEST_SUITE("Bitvec") {

    // ========================================================================
    // Construction
    // ========================================================================

    TEST_CASE("DefaultConstruction") {
        Bitvec bv;
        CHECK(bv.size() == 0);
        CHECK(bv.empty());
    }

    TEST_CASE("SizeConstruction") {
        Bitvec bv(10);
        CHECK(bv.size() == 10);
        CHECK_FALSE(bv.empty());
        CHECK(bv.none()); // All bits should be 0
    }

    TEST_CASE("StringConstruction") {
        Bitvec bv("10101");
        CHECK(bv.size() == 5);
        CHECK(bv.test(0));
        CHECK_FALSE(bv.test(1));
        CHECK(bv.test(2));
        CHECK_FALSE(bv.test(3));
        CHECK(bv.test(4));
    }

    // ========================================================================
    // Element Access
    // ========================================================================

    TEST_CASE("Test") {
        Bitvec bv(10);
        bv.set(3, true);
        bv.set(7, true);

        CHECK(bv.test(3));
        CHECK(bv.test(7));
        CHECK_FALSE(bv.test(0));
        CHECK_FALSE(bv.test(5));
    }

    TEST_CASE("OperatorBracket") {
        Bitvec bv(10);
        bv.set(5, true);

        CHECK(bv[5]);
        CHECK_FALSE(bv[0]);
    }

    TEST_CASE("TestOutOfBounds") {
        Bitvec bv(5);
        CHECK_FALSE(bv.test(100)); // Should return false for out of bounds
    }

    // ========================================================================
    // Modifiers - Set/Reset
    // ========================================================================

    TEST_CASE("Set") {
        Bitvec bv(10);
        bv.set(0, true);
        bv.set(5, true);
        bv.set(9, true);

        CHECK(bv.test(0));
        CHECK(bv.test(5));
        CHECK(bv.test(9));
        CHECK(bv.count() == 3);
    }

    TEST_CASE("SetFalse") {
        Bitvec bv(10);
        bv.set(3, true);
        CHECK(bv.test(3));

        bv.set(3, false);
        CHECK_FALSE(bv.test(3));
    }

    TEST_CASE("SetFromString") {
        Bitvec bv;
        bv.set("11110000");

        CHECK(bv.size() == 8);
        CHECK(bv.test(4));
        CHECK(bv.test(5));
        CHECK(bv.test(6));
        CHECK(bv.test(7));
        CHECK_FALSE(bv.test(0));
        CHECK_FALSE(bv.test(3));
    }

    TEST_CASE("Reset") {
        Bitvec bv(10);
        bv.set(0, true);
        bv.set(5, true);
        CHECK(bv.count() == 2);

        bv.reset();
        CHECK(bv.empty());
        CHECK(bv.size() == 0);
    }

    // ========================================================================
    // Modifiers - Push/Pop
    // ========================================================================

    TEST_CASE("PushBack") {
        Bitvec bv;
        bv.push_back(true);
        bv.push_back(false);
        bv.push_back(true);

        CHECK(bv.size() == 3);
        CHECK(bv.test(0));
        CHECK_FALSE(bv.test(1));
        CHECK(bv.test(2));
    }

    TEST_CASE("PopBack") {
        Bitvec bv;
        bv.push_back(true);
        bv.push_back(false);
        bv.push_back(true);
        CHECK(bv.size() == 3);

        bv.pop_back();
        CHECK(bv.size() == 2);
        CHECK(bv.test(0));
        CHECK_FALSE(bv.test(1));
    }

    TEST_CASE("PopBackEmpty") {
        Bitvec bv;
        bv.pop_back(); // Should not crash on empty
        CHECK(bv.empty());
    }

    // ========================================================================
    // Modifiers - Flip
    // ========================================================================

    TEST_CASE("FlipSingleBit") {
        Bitvec bv(10);
        bv.flip(3);
        CHECK(bv.test(3));
        CHECK(bv.count() == 1);

        bv.flip(3);
        CHECK_FALSE(bv.test(3));
        CHECK(bv.count() == 0);
    }

    TEST_CASE("FlipAllBits") {
        Bitvec bv("10101");
        bv.flip();

        CHECK(bv.str() == "01010");
        CHECK_FALSE(bv.test(0));
        CHECK(bv.test(1));
        CHECK_FALSE(bv.test(2));
        CHECK(bv.test(3));
        CHECK_FALSE(bv.test(4));
    }

    // ========================================================================
    // Capacity
    // ========================================================================

    TEST_CASE("Size") {
        Bitvec bv(100);
        CHECK(bv.size() == 100);
    }

    TEST_CASE("Empty") {
        Bitvec bv;
        CHECK(bv.empty());

        bv.push_back(true);
        CHECK_FALSE(bv.empty());
    }

    TEST_CASE("Resize") {
        Bitvec bv(5);
        bv.set(2, true);
        CHECK(bv.size() == 5);

        bv.resize(10);
        CHECK(bv.size() == 10);
        CHECK(bv.test(2));       // Old bit should still be set
        CHECK_FALSE(bv.test(7)); // New bits should be 0
    }

    TEST_CASE("ResizeShrink") {
        Bitvec bv(10);
        bv.set(8, true);
        CHECK(bv.size() == 10);

        bv.resize(5);
        CHECK(bv.size() == 5);
        CHECK(bv.none()); // Bit 8 should be gone
    }

    TEST_CASE("Reserve") {
        Bitvec bv;
        bv.reserve(1000);
        CHECK(bv.capacity() >= 1000);
        CHECK(bv.size() == 0); // Size shouldn't change
    }

    TEST_CASE("Capacity") {
        Bitvec bv;
        auto initial_cap = bv.capacity();

        bv.reserve(5000);
        CHECK(bv.capacity() >= 5000);
        CHECK(bv.capacity() >= initial_cap);
    }

    TEST_CASE("Clear") {
        Bitvec bv(10);
        bv.set(3, true);
        bv.set(7, true);
        CHECK(bv.size() == 10);

        bv.clear();
        CHECK(bv.size() == 0);
        CHECK(bv.empty());
    }

    // ========================================================================
    // Query Operations
    // ========================================================================

    TEST_CASE("Count") {
        Bitvec bv(10);
        CHECK(bv.count() == 0);

        bv.set(0, true);
        bv.set(5, true);
        bv.set(9, true);
        CHECK(bv.count() == 3);
    }

    TEST_CASE("CountEmpty") {
        Bitvec bv;
        CHECK(bv.count() == 0);
    }

    TEST_CASE("Any") {
        Bitvec bv(10);
        CHECK_FALSE(bv.any());

        bv.set(5, true);
        CHECK(bv.any());
    }

    TEST_CASE("AnyEmpty") {
        Bitvec bv;
        CHECK_FALSE(bv.any());
    }

    TEST_CASE("None") {
        Bitvec bv(10);
        CHECK(bv.none());

        bv.set(5, true);
        CHECK_FALSE(bv.none());
    }

    TEST_CASE("NoneEmpty") {
        Bitvec bv;
        CHECK(bv.none());
    }

    // ========================================================================
    // String Conversion
    // ========================================================================

    TEST_CASE("Str") {
        Bitvec bv("10101010");
        CHECK(bv.str() == "10101010");
    }

    TEST_CASE("StrEmpty") {
        Bitvec bv;
        CHECK(bv.str() == "");
    }

    // ========================================================================
    // Bitwise Operations
    // ========================================================================

    TEST_CASE("BitwiseAnd") {
        Bitvec bv1("11110000");
        Bitvec bv2("10101010");

        bv1 &= bv2;
        CHECK(bv1.str() == "10100000");
    }

    TEST_CASE("BitwiseOr") {
        Bitvec bv1("11110000");
        Bitvec bv2("10101010");

        bv1 |= bv2;
        CHECK(bv1.str() == "11111010");
    }

    TEST_CASE("BitwiseXor") {
        Bitvec bv1("11110000");
        Bitvec bv2("10101010");

        bv1 ^= bv2;
        CHECK(bv1.str() == "01011010");
    }

    TEST_CASE("BitwiseNot") {
        Bitvec bv("10101010");
        auto result = ~bv;

        CHECK(result.str() == "01010101");
        CHECK(bv.str() == "10101010"); // Original unchanged
    }

    // ========================================================================
    // Comparison
    // ========================================================================

    TEST_CASE("Equality") {
        Bitvec bv1("10101");
        Bitvec bv2("10101");
        Bitvec bv3("01010");

        CHECK(bv1 == bv2);
        CHECK_FALSE(bv1 == bv3);
        CHECK(bv1 != bv3);
    }

    TEST_CASE("EqualityEmpty") {
        Bitvec bv1;
        Bitvec bv2;
        CHECK(bv1 == bv2);
    }

    TEST_CASE("EqualityDifferentSizes") {
        Bitvec bv1(5);
        Bitvec bv2(10);
        CHECK_FALSE(bv1 == bv2);
    }

    // ========================================================================
    // Iteration
    // ========================================================================

    TEST_CASE("ForEachSetBit") {
        Bitvec bv("10001000");

        std::vector<std::size_t> indices;
        bv.for_each_set_bit([&](auto i) { indices.push_back(i); });

        REQUIRE(indices.size() == 2);
        CHECK(indices[0] == 3);
        CHECK(indices[1] == 7);
    }

    TEST_CASE("ForEachSetBitEmpty") {
        Bitvec bv;

        std::size_t count = 0;
        bv.for_each_set_bit([&](auto) { count++; });

        CHECK(count == 0);
    }

    TEST_CASE("NextSetBit") {
        Bitvec bv("10001000");

        auto idx1 = bv.next_set_bit(0);
        REQUIRE(idx1.has_value());
        CHECK(*idx1 == 3);

        auto idx2 = bv.next_set_bit(*idx1 + 1);
        REQUIRE(idx2.has_value());
        CHECK(*idx2 == 7);

        auto idx3 = bv.next_set_bit(*idx2 + 1);
        CHECK_FALSE(idx3.has_value());
    }

    // ========================================================================
    // Serialization
    // ========================================================================

    TEST_CASE("Members") {
        Bitvec bv(10);
        bv.set(3, true);
        bv.set(7, true);

        auto [size, blocks] = bv.members();
        CHECK(size == 10);
        CHECK(blocks.size() > 0);
    }

    // ========================================================================
    // Edge Cases
    // ========================================================================

    TEST_CASE("LargeBitvec") {
        Bitvec bv(10000);
        CHECK(bv.size() == 10000);
        CHECK(bv.none());

        bv.set(5000, true);
        CHECK(bv.test(5000));
        CHECK(bv.count() == 1);
    }

    TEST_CASE("SingleBit") {
        Bitvec bv(1);
        CHECK(bv.size() == 1);
        CHECK_FALSE(bv.test(0));

        bv.set(0, true);
        CHECK(bv.test(0));
        CHECK(bv.any());
        CHECK_FALSE(bv.none());
    }

    TEST_CASE("MultipleBlockBoundary") {
        // Test across 64-bit block boundaries
        Bitvec bv(200);
        bv.set(63, true);  // Last bit of first block
        bv.set(64, true);  // First bit of second block
        bv.set(127, true); // Last bit of second block
        bv.set(128, true); // First bit of third block

        CHECK(bv.test(63));
        CHECK(bv.test(64));
        CHECK(bv.test(127));
        CHECK(bv.test(128));
        CHECK(bv.count() == 4);
    }

    TEST_CASE("PushBackManyBits") {
        Bitvec bv;
        for (int i = 0; i < 100; ++i) {
            bv.push_back(i % 2 == 0);
        }

        CHECK(bv.size() == 100);
        CHECK(bv.test(0));       // Even index -> true
        CHECK_FALSE(bv.test(1)); // Odd index -> false
        CHECK(bv.test(98));
        CHECK_FALSE(bv.test(99));
    }

    TEST_CASE("ClearAndReuse") {
        Bitvec bv(100);
        bv.set(50, true);
        CHECK(bv.size() == 100);

        bv.clear();
        CHECK(bv.empty());

        bv.push_back(true);
        CHECK(bv.size() == 1);
        CHECK(bv.test(0));
    }
}
