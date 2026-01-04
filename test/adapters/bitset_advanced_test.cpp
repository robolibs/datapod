#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "datapod/pods/adapters/bitset.hpp"

using namespace datapod;

TEST_SUITE("Bitset Advanced Operations") {
    TEST_CASE("Bitset - count_ones") {
        Bitset<8> b;
        CHECK(b.count_ones() == 0);

        b.set(0);
        CHECK(b.count_ones() == 1);

        b.set(3);
        b.set(7);
        CHECK(b.count_ones() == 3);

        b.set();
        CHECK(b.count_ones() == 8);
    }

    TEST_CASE("Bitset - count_zeros") {
        Bitset<8> b;
        CHECK(b.count_zeros() == 8);

        b.set(0);
        CHECK(b.count_zeros() == 7);

        b.set(3);
        b.set(7);
        CHECK(b.count_zeros() == 5);

        b.set();
        CHECK(b.count_zeros() == 0);
    }

    TEST_CASE("Bitset - count_ones and count_zeros are complementary") {
        Bitset<16> b;
        for (std::size_t i = 0; i < 16; ++i) {
            CHECK(b.count_ones() + b.count_zeros() == 16);
            b.set(i);
        }
        CHECK(b.count_ones() + b.count_zeros() == 16);
    }

    TEST_CASE("Bitset - leading_zeros with all zeros") {
        Bitset<8> b;
        CHECK(b.leading_zeros() == 8);

        Bitset<64> b64;
        CHECK(b64.leading_zeros() == 64);

        Bitset<128> b128;
        CHECK(b128.leading_zeros() == 128);
    }

    TEST_CASE("Bitset - leading_zeros with LSB set") {
        Bitset<8> b;
        b.set(0); // 00000001
        CHECK(b.leading_zeros() == 7);

        Bitset<16> b16;
        b16.set(0);
        CHECK(b16.leading_zeros() == 15);
    }

    TEST_CASE("Bitset - leading_zeros with MSB set") {
        Bitset<8> b;
        b.set(7); // 10000000
        CHECK(b.leading_zeros() == 0);

        Bitset<16> b16;
        b16.set(15);
        CHECK(b16.leading_zeros() == 0);
    }

    TEST_CASE("Bitset - leading_zeros with middle bit set") {
        Bitset<8> b;
        b.set(3); // 00001000
        CHECK(b.leading_zeros() == 4);

        Bitset<16> b16;
        b16.set(10); // bit 10 set
        CHECK(b16.leading_zeros() == 5);
    }

    TEST_CASE("Bitset - trailing_zeros with all zeros") {
        Bitset<8> b;
        CHECK(b.trailing_zeros() == 8);

        Bitset<64> b64;
        CHECK(b64.trailing_zeros() == 64);

        Bitset<128> b128;
        CHECK(b128.trailing_zeros() == 128);
    }

    TEST_CASE("Bitset - trailing_zeros with LSB set") {
        Bitset<8> b;
        b.set(0); // 00000001
        CHECK(b.trailing_zeros() == 0);

        Bitset<16> b16;
        b16.set(0);
        CHECK(b16.trailing_zeros() == 0);
    }

    TEST_CASE("Bitset - trailing_zeros with MSB set") {
        Bitset<8> b;
        b.set(7); // 10000000
        CHECK(b.trailing_zeros() == 7);

        Bitset<16> b16;
        b16.set(15);
        CHECK(b16.trailing_zeros() == 15);
    }

    TEST_CASE("Bitset - trailing_zeros with middle bit set") {
        Bitset<8> b;
        b.set(3); // 00001000
        CHECK(b.trailing_zeros() == 3);

        Bitset<16> b16;
        b16.set(5);
        CHECK(b16.trailing_zeros() == 5);
    }

    TEST_CASE("Bitset - trailing_zeros with multiple bits") {
        Bitset<8> b;
        b.set(2);                       // 00000100
        b.set(5);                       // 00100100
        CHECK(b.trailing_zeros() == 2); // First 1 bit is at position 2
    }

    TEST_CASE("Bitset - rotate_left basic") {
        Bitset<8> b;
        b.set(0); // 00000001

        b.rotate_left(1);
        CHECK(b.test(1) == true);
        CHECK(b.test(0) == false);
        CHECK(b.to_string() == "00000010");
    }

    TEST_CASE("Bitset - rotate_left wrap around") {
        Bitset<8> b;
        b.set(7); // 10000000

        b.rotate_left(1);
        CHECK(b.test(0) == true);
        CHECK(b.test(7) == false);
        CHECK(b.to_string() == "00000001");
    }

    TEST_CASE("Bitset - rotate_left multiple positions") {
        Bitset<8> b;
        b.set(0); // 00000001

        b.rotate_left(3);
        CHECK(b.test(3) == true);
        CHECK(b.to_string() == "00001000");
    }

    TEST_CASE("Bitset - rotate_left full rotation") {
        Bitset<8> b;
        b.set(0);
        b.set(3);
        auto original = b.to_string();

        b.rotate_left(8);
        CHECK(b.to_string() == original);
    }

    TEST_CASE("Bitset - rotate_left complex pattern") {
        Bitset<8> b;
        b.set(0);
        b.set(1);
        b.set(2); // 00000111

        b.rotate_left(2);
        CHECK(b.to_string() == "00011100");
    }

    TEST_CASE("Bitset - rotate_right basic") {
        Bitset<8> b;
        b.set(1); // 00000010

        b.rotate_right(1);
        CHECK(b.test(0) == true);
        CHECK(b.test(1) == false);
        CHECK(b.to_string() == "00000001");
    }

    TEST_CASE("Bitset - rotate_right wrap around") {
        Bitset<8> b;
        b.set(0); // 00000001

        b.rotate_right(1);
        CHECK(b.test(7) == true);
        CHECK(b.test(0) == false);
        CHECK(b.to_string() == "10000000");
    }

    TEST_CASE("Bitset - rotate_right multiple positions") {
        Bitset<8> b;
        b.set(7); // 10000000

        b.rotate_right(3);
        CHECK(b.test(4) == true);
        CHECK(b.to_string() == "00010000");
    }

    TEST_CASE("Bitset - rotate_right full rotation") {
        Bitset<8> b;
        b.set(2);
        b.set(5);
        auto original = b.to_string();

        b.rotate_right(8);
        CHECK(b.to_string() == original);
    }

    TEST_CASE("Bitset - rotate_right complex pattern") {
        Bitset<8> b;
        b.set(5);
        b.set(6);
        b.set(7); // 11100000

        b.rotate_right(2);
        CHECK(b.to_string() == "00111000");
    }

    TEST_CASE("Bitset - rotate_left and rotate_right are inverses") {
        Bitset<16> b;
        b.set(3);
        b.set(7);
        b.set(12);
        auto original = b.to_string();

        b.rotate_left(5);
        b.rotate_right(5);
        CHECK(b.to_string() == original);
    }

    TEST_CASE("Bitset - rotate with large bitset") {
        Bitset<128> b;
        b.set(0);
        b.set(64);
        b.set(127);

        b.rotate_left(1);
        CHECK(b.test(1) == true);
        CHECK(b.test(65) == true);
        CHECK(b.test(0) == true); // wrapped from 127
        CHECK(b.test(127) == false);
    }

    TEST_CASE("Bitset - rotate_left by zero") {
        Bitset<8> b;
        b.set(3);
        auto original = b.to_string();

        b.rotate_left(0);
        CHECK(b.to_string() == original);
    }

    TEST_CASE("Bitset - rotate_right by zero") {
        Bitset<8> b;
        b.set(5);
        auto original = b.to_string();

        b.rotate_right(0);
        CHECK(b.to_string() == original);
    }

    TEST_CASE("Bitset - rotate with modulo normalization") {
        Bitset<8> b;
        b.set(0);

        b.rotate_left(9); // Should be same as rotate_left(1)
        CHECK(b.test(1) == true);
        CHECK(b.test(0) == false);
    }

    TEST_CASE("Bitset - leading_zeros with multi-block bitset") {
        Bitset<128> b;
        b.set(100);
        CHECK(b.leading_zeros() == 27); // 128 - 100 - 1
    }

    TEST_CASE("Bitset - trailing_zeros with multi-block bitset") {
        Bitset<128> b;
        b.set(100);
        CHECK(b.trailing_zeros() == 100);
    }

    TEST_CASE("Bitset - count operations with multi-block") {
        Bitset<128> b;
        for (std::size_t i = 0; i < 128; i += 2) {
            b.set(i);
        }
        CHECK(b.count_ones() == 64);
        CHECK(b.count_zeros() == 64);
    }
}
