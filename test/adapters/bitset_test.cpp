#include <doctest/doctest.h>

#include "datapod/adapters/bitset.hpp"

using namespace datapod;

TEST_SUITE("Bitset") {

    // ========================================================================
    // Construction and Basic Access
    // ========================================================================

    TEST_CASE("DefaultConstruction") {
        Bitset<8> bs;
        CHECK(bs.size() == 8U);
        CHECK(bs.none());
        CHECK_FALSE(bs.any());
        CHECK(bs.count() == 0U);
    }

    TEST_CASE("StringConstruction") {
        Bitset<8> bs1("10101010");
        CHECK(bs1.test(1));
        CHECK(bs1.test(3));
        CHECK(bs1.test(5));
        CHECK(bs1.test(7));
        CHECK_FALSE(bs1.test(0));
        CHECK_FALSE(bs1.test(2));

        Bitset<16> bs2("1111000011110000");
        CHECK(bs2.count() == 8U);
    }

    TEST_CASE("MaxConstruction") {
        auto bs = Bitset<8>::max();
        CHECK(bs.all());
        CHECK(bs.count() == 8U);
    }

    // ========================================================================
    // Set, Reset, Flip Operations
    // ========================================================================

    TEST_CASE("SetSingleBit") {
        Bitset<8> bs;
        bs.set(3);
        CHECK(bs.test(3));
        CHECK(bs.count() == 1U);

        bs.set(5, true);
        CHECK(bs.test(5));
        CHECK(bs.count() == 2U);

        bs.set(3, false);
        CHECK_FALSE(bs.test(3));
        CHECK(bs.count() == 1U);
    }

    TEST_CASE("SetAllBits") {
        Bitset<8> bs;
        bs.set();
        CHECK(bs.all());
        CHECK(bs.count() == 8U);
        for (std::size_t i = 0; i < 8; ++i) {
            CHECK(bs.test(i));
        }
    }

    TEST_CASE("SetAllBitsLarge") {
        Bitset<100> bs;
        bs.set();
        CHECK(bs.all());
        CHECK(bs.count() == 100U);
    }

    TEST_CASE("ResetSingleBit") {
        Bitset<8> bs;
        bs.set();
        bs.reset(3);
        CHECK_FALSE(bs.test(3));
        CHECK(bs.count() == 7U);
    }

    TEST_CASE("ResetAllBits") {
        Bitset<8> bs;
        bs.set();
        bs.reset();
        CHECK(bs.none());
        CHECK(bs.count() == 0U);
    }

    TEST_CASE("FlipSingleBit") {
        Bitset<8> bs;
        bs.flip(3);
        CHECK(bs.test(3));
        CHECK(bs.count() == 1U);

        bs.flip(3);
        CHECK_FALSE(bs.test(3));
        CHECK(bs.count() == 0U);
    }

    TEST_CASE("FlipAllBits") {
        Bitset<8> bs("10101010");
        bs.flip();
        CHECK(bs.to_string() == "01010101");

        bs.flip();
        CHECK(bs.to_string() == "10101010");
    }

    TEST_CASE("FlipAllBitsLarge") {
        Bitset<100> bs;
        bs.flip();
        CHECK(bs.all());
        CHECK(bs.count() == 100U);
    }

    // ========================================================================
    // Query Operations
    // ========================================================================

    TEST_CASE("All") {
        Bitset<8> bs;
        CHECK_FALSE(bs.all());

        bs.set();
        CHECK(bs.all());

        bs.reset(3);
        CHECK_FALSE(bs.all());
    }

    TEST_CASE("AllPartialBlock") {
        Bitset<65> bs; // More than one block, partial last block
        CHECK_FALSE(bs.all());

        bs.set();
        CHECK(bs.all());
        CHECK(bs.count() == 65U);

        bs.reset(64);
        CHECK_FALSE(bs.all());
    }

    TEST_CASE("Any") {
        Bitset<8> bs;
        CHECK_FALSE(bs.any());

        bs.set(3);
        CHECK(bs.any());

        bs.reset();
        CHECK_FALSE(bs.any());
    }

    TEST_CASE("None") {
        Bitset<8> bs;
        CHECK(bs.none());

        bs.set(3);
        CHECK_FALSE(bs.none());

        bs.reset();
        CHECK(bs.none());
    }

    TEST_CASE("Count") {
        Bitset<8> bs;
        CHECK(bs.count() == 0U);

        bs.set(0);
        bs.set(3);
        bs.set(7);
        CHECK(bs.count() == 3U);

        bs.set();
        CHECK(bs.count() == 8U);
    }

    TEST_CASE("CountLarge") {
        Bitset<128> bs;
        for (std::size_t i = 0; i < 128; i += 2) {
            bs.set(i);
        }
        CHECK(bs.count() == 64U);
    }

    TEST_CASE("Test") {
        Bitset<8> bs;
        bs.set(3);
        CHECK(bs.test(3));
        CHECK_FALSE(bs.test(0));
        CHECK_FALSE(bs.test(100)); // Out of bounds returns false
    }

    // ========================================================================
    // Conversion Operations
    // ========================================================================

    TEST_CASE("ToString") {
        Bitset<8> bs("10101010");
        CHECK(bs.to_string() == "10101010");

        Bitset<4> bs2("1111");
        CHECK(bs2.to_string() == "1111");
    }

    TEST_CASE("ToULong") {
        Bitset<8> bs("00000101"); // 5 in binary
        CHECK(bs.to_ulong() == 5UL);

        Bitset<16> bs2("0000000000001111"); // 15
        CHECK(bs2.to_ulong() == 15UL);

        Bitset<64> bs3;
        bs3.set(0);
        bs3.set(1);
        CHECK(bs3.to_ulong() == 3UL);
    }

    TEST_CASE("ToULongSmall") {
        Bitset<4> bs;
        CHECK(bs.to_ulong() == 0UL);
        bs.set();
        CHECK(bs.to_ulong() == 15UL);
    }

    TEST_CASE("ToULongOverflow") {
        Bitset<128> bs;
        bs.set(100); // Set bit beyond 64
        CHECK_THROWS_AS(bs.to_ulong(), std::overflow_error);
    }

    TEST_CASE("ToULLong") {
        Bitset<8> bs("00000101"); // 5
        CHECK(bs.to_ullong() == 5ULL);

        Bitset<64> bs2;
        bs2.set(0);
        bs2.set(63);
        CHECK(bs2.to_ullong() == (1ULL | (1ULL << 63)));
    }

    TEST_CASE("ToULLongSmall") {
        Bitset<4> bs;
        CHECK(bs.to_ullong() == 0ULL);
        bs.set();
        CHECK(bs.to_ullong() == 15ULL);
    }

    TEST_CASE("ToULLongOverflow") {
        Bitset<128> bs;
        bs.set(100); // Set bit beyond 64
        CHECK_THROWS_AS(bs.to_ullong(), std::overflow_error);
    }

    // ========================================================================
    // Bitwise Operations
    // ========================================================================

    TEST_CASE("BitwiseAnd") {
        Bitset<8> bs1("11110000");
        Bitset<8> bs2("10101010");
        auto result = bs1 & bs2;
        CHECK(result.to_string() == "10100000");
    }

    TEST_CASE("BitwiseOr") {
        Bitset<8> bs1("11110000");
        Bitset<8> bs2("10101010");
        auto result = bs1 | bs2;
        CHECK(result.to_string() == "11111010");
    }

    TEST_CASE("BitwiseXor") {
        Bitset<8> bs1("11110000");
        Bitset<8> bs2("10101010");
        auto result = bs1 ^ bs2;
        CHECK(result.to_string() == "01011010");
    }

    TEST_CASE("BitwiseNot") {
        Bitset<8> bs("10101010");
        auto result = ~bs;
        CHECK(result.to_string() == "01010101");
    }

    TEST_CASE("BitwiseAndAssign") {
        Bitset<8> bs1("11110000");
        Bitset<8> bs2("10101010");
        bs1 &= bs2;
        CHECK(bs1.to_string() == "10100000");
    }

    TEST_CASE("BitwiseOrAssign") {
        Bitset<8> bs1("11110000");
        Bitset<8> bs2("10101010");
        bs1 |= bs2;
        CHECK(bs1.to_string() == "11111010");
    }

    TEST_CASE("BitwiseXorAssign") {
        Bitset<8> bs1("11110000");
        Bitset<8> bs2("10101010");
        bs1 ^= bs2;
        CHECK(bs1.to_string() == "01011010");
    }

    // ========================================================================
    // Shift Operations
    // ========================================================================

    TEST_CASE("LeftShift") {
        Bitset<8> bs("00000011");
        auto result = bs << 2;
        CHECK(result.to_string() == "00001100");
    }

    TEST_CASE("RightShift") {
        Bitset<8> bs("11000000");
        auto result = bs >> 2;
        CHECK(result.to_string() == "00110000");
    }

    TEST_CASE("LeftShiftAssign") {
        Bitset<8> bs("00000011");
        bs <<= 2;
        CHECK(bs.to_string() == "00001100");
    }

    TEST_CASE("RightShiftAssign") {
        Bitset<8> bs("11000000");
        bs >>= 2;
        CHECK(bs.to_string() == "00110000");
    }

    TEST_CASE("ShiftOverflow") {
        Bitset<8> bs("11111111");
        bs <<= 10;
        CHECK(bs.to_string() == "00000000");

        Bitset<8> bs2("11111111");
        bs2 >>= 10;
        CHECK(bs2.to_string() == "00000000");
    }

    // ========================================================================
    // Comparison Operations
    // ========================================================================

    TEST_CASE("Equality") {
        Bitset<8> bs1("10101010");
        Bitset<8> bs2("10101010");
        Bitset<8> bs3("01010101");

        CHECK(bs1 == bs2);
        CHECK_FALSE(bs1 == bs3);
        CHECK(bs1 != bs3);
        CHECK_FALSE(bs1 != bs2);
    }

    TEST_CASE("Comparison") {
        Bitset<8> bs1("00000001");
        Bitset<8> bs2("00000010");

        CHECK(bs1 < bs2);
        CHECK(bs2 > bs1);
        CHECK(bs1 <= bs2);
        CHECK(bs2 >= bs1);
        CHECK(bs1 <= bs1);
        CHECK(bs1 >= bs1);
    }

    // ========================================================================
    // Indexing
    // ========================================================================

    TEST_CASE("SubscriptOperator") {
        Bitset<8> bs("10101010");
        CHECK(bs[1]);
        CHECK(bs[3]);
        CHECK_FALSE(bs[0]);
        CHECK_FALSE(bs[2]);
    }

    // ========================================================================
    // Edge Cases
    // ========================================================================

    TEST_CASE("VerySmallBitset") {
        Bitset<1> bs;
        CHECK(bs.size() == 1U);
        CHECK(bs.count() == 0U);
        CHECK(bs.none());
        CHECK_FALSE(bs.any());
    }

    TEST_CASE("SingleBit") {
        Bitset<1> bs;
        CHECK_FALSE(bs.test(0));
        bs.set(0);
        CHECK(bs.test(0));
        CHECK(bs.all());
        CHECK(bs.to_string() == "1");
    }

    TEST_CASE("LargeBitset") {
        Bitset<1024> bs;
        bs.set(0);
        bs.set(512);
        bs.set(1023);
        CHECK(bs.count() == 3U);
        CHECK(bs.test(0));
        CHECK(bs.test(512));
        CHECK(bs.test(1023));
        CHECK_FALSE(bs.test(511));
    }

    TEST_CASE("PartialBlock") {
        Bitset<65> bs; // 2 blocks, second is partial
        bs.set();
        CHECK(bs.all());
        CHECK(bs.count() == 65U);

        bs.flip();
        CHECK(bs.none());
    }

    // ========================================================================
    // Helper Functions
    // ========================================================================

    TEST_CASE("ZeroOut") {
        Bitset<8> bs;
        bs.set();
        bs.zero_out();
        CHECK(bs.none());
    }

    TEST_CASE("OneOut") {
        Bitset<8> bs;
        bs.one_out();
        CHECK(bs.all());
    }

    TEST_CASE("ForEachSetBit") {
        Bitset<8> bs("10101010");
        std::size_t count = 0;
        bs.for_each_set_bit([&](std::size_t i) {
            count++;
            CHECK(bs.test(i));
        });
        CHECK(count == 4U);
    }

    // ========================================================================
    // Chaining Operations
    // ========================================================================

    TEST_CASE("MethodChaining") {
        Bitset<8> bs;
        bs.set().flip(0).flip(2).reset(7);
        CHECK_FALSE(bs.test(0));
        CHECK(bs.test(1));
        CHECK_FALSE(bs.test(2));
        CHECK_FALSE(bs.test(7));
    }

    TEST_CASE("ComplexChaining") {
        Bitset<16> bs;
        bs.set().flip().set(0).set(15);
        CHECK(bs.test(0));
        CHECK(bs.test(15));
        CHECK_FALSE(bs.test(1));
        CHECK_FALSE(bs.test(14));
    }
}
