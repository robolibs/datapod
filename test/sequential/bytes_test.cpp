#include <doctest/doctest.h>

#include "datapod/pods/sequential/bytes.hpp"

#include <cstring>

using namespace datapod;

TEST_SUITE("Bytes") {

    // ========================================================================
    // Construction
    // ========================================================================

    TEST_CASE("DefaultConstruction") {
        Bytes b;
        CHECK(b.size() == 0);
        CHECK(b.empty());
    }

    TEST_CASE("SizeConstruction") {
        Bytes b(10);
        CHECK(b.size() == 10);
        CHECK_FALSE(b.empty());
    }

    TEST_CASE("SizeValueConstruction") {
        Bytes b(10, 0xFF);
        CHECK(b.size() == 10);
        for (size_t i = 0; i < 10; ++i) {
            CHECK(b[i] == 0xFF);
        }
    }

    TEST_CASE("RawPointerConstruction") {
        u8 raw_data[] = {0x01, 0x02, 0x03, 0x04};
        Bytes b(raw_data, 4);
        CHECK(b.size() == 4);
        CHECK(b[0] == 0x01);
        CHECK(b[1] == 0x02);
        CHECK(b[2] == 0x03);
        CHECK(b[3] == 0x04);
    }

    TEST_CASE("VoidPointerConstruction") {
        u8 raw_data[] = {0xAA, 0xBB, 0xCC};
        Bytes b(static_cast<void *>(raw_data), 3);
        CHECK(b.size() == 3);
        CHECK(b[0] == 0xAA);
        CHECK(b[1] == 0xBB);
        CHECK(b[2] == 0xCC);
    }

    TEST_CASE("InitializerListConstruction") {
        Bytes b = {0x00, 0x11, 0x22, 0x33, 0x44};
        CHECK(b.size() == 5);
        CHECK(b[0] == 0x00);
        CHECK(b[2] == 0x22);
        CHECK(b[4] == 0x44);
    }

    TEST_CASE("RangeConstruction") {
        std::vector<u8> v = {0x10, 0x20, 0x30, 0x40};
        Bytes b(v.begin(), v.end());
        CHECK(b.size() == 4);
        CHECK(b[0] == 0x10);
        CHECK(b[3] == 0x40);
    }

    // ========================================================================
    // Element Access
    // ========================================================================

    TEST_CASE("OperatorBracket") {
        Bytes b(10);
        b[0] = 0x12;
        b[5] = 0x34;
        b[9] = 0x56;

        CHECK(b[0] == 0x12);
        CHECK(b[5] == 0x34);
        CHECK(b[9] == 0x56);
    }

    TEST_CASE("At") {
        Bytes b(10);
        b.at(3) = 0xAB;
        CHECK(b.at(3) == 0xAB);
    }

    TEST_CASE("AtOutOfBounds") {
        Bytes b(5);
        CHECK_THROWS_AS(b.at(10), std::out_of_range);
    }

    TEST_CASE("Front") {
        Bytes b = {0x01, 0x02, 0x03};
        CHECK(b.front() == 0x01);
    }

    TEST_CASE("Back") {
        Bytes b = {0x01, 0x02, 0x03};
        CHECK(b.back() == 0x03);
    }

    TEST_CASE("Data") {
        Bytes b = {0x10, 0x20, 0x30};
        u8 *ptr = b.data();
        CHECK(ptr[0] == 0x10);
        CHECK(ptr[1] == 0x20);
        CHECK(ptr[2] == 0x30);
    }

    TEST_CASE("VoidData") {
        Bytes b = {0xAA, 0xBB, 0xCC};
        void *ptr = b.void_data();
        u8 *uptr = static_cast<u8 *>(ptr);
        CHECK(uptr[0] == 0xAA);
        CHECK(uptr[1] == 0xBB);
        CHECK(uptr[2] == 0xCC);
    }

    // ========================================================================
    // Iterators
    // ========================================================================

    TEST_CASE("Iterators") {
        Bytes b = {0x01, 0x02, 0x03, 0x04};

        auto sum = u8{0};
        for (auto byte : b) {
            sum += byte;
        }
        CHECK(sum == 0x0A); // 0x01 + 0x02 + 0x03 + 0x04 = 0x0A (10)
    }

    // ========================================================================
    // Capacity
    // ========================================================================

    TEST_CASE("Size") {
        Bytes b(100);
        CHECK(b.size() == 100);
    }

    TEST_CASE("Empty") {
        Bytes b;
        CHECK(b.empty());

        b.push_back(0x01);
        CHECK_FALSE(b.empty());
    }

    TEST_CASE("Capacity") {
        Bytes b;
        b.reserve(1000);
        CHECK(b.capacity() >= 1000);
        CHECK(b.size() == 0);
    }

    TEST_CASE("Resize") {
        Bytes b(5);
        b[2] = 0xFF;
        CHECK(b.size() == 5);

        b.resize(10);
        CHECK(b.size() == 10);
        CHECK(b[2] == 0xFF); // Old byte should still be there
        CHECK(b[5] == 0x00); // New bytes should be 0
    }

    TEST_CASE("ResizeWithValue") {
        Bytes b(3);
        b.resize(10, 0xAB);
        CHECK(b.size() == 10);
        CHECK(b[5] == 0xAB);
        CHECK(b[9] == 0xAB);
    }

    TEST_CASE("ResizeShrink") {
        Bytes b(10);
        b[8] = 0xFF;
        CHECK(b.size() == 10);

        b.resize(5);
        CHECK(b.size() == 5);
    }

    TEST_CASE("ShrinkToFit") {
        Bytes b;
        b.reserve(1000);
        b.resize(10);
        auto cap_before = b.capacity();

        b.shrink_to_fit();
        CHECK(b.capacity() <= cap_before);
        CHECK(b.size() == 10);
    }

    // ========================================================================
    // Modifiers
    // ========================================================================

    TEST_CASE("PushBack") {
        Bytes b;
        b.push_back(0x01);
        b.push_back(0x02);
        b.push_back(0x03);

        CHECK(b.size() == 3);
        CHECK(b[0] == 0x01);
        CHECK(b[1] == 0x02);
        CHECK(b[2] == 0x03);
    }

    TEST_CASE("PopBack") {
        Bytes b = {0x01, 0x02, 0x03, 0x04};
        CHECK(b.size() == 4);

        b.pop_back();
        CHECK(b.size() == 3);
        CHECK(b.back() == 0x03);
    }

    TEST_CASE("EmplaceBack") {
        Bytes b;
        b.emplace_back(0xFF);
        CHECK(b.size() == 1);
        CHECK(b[0] == 0xFF);
    }

    TEST_CASE("AppendRawBytes") {
        Bytes b = {0x01, 0x02};
        u8 more[] = {0x03, 0x04, 0x05};
        b.append(more, 3);

        CHECK(b.size() == 5);
        CHECK(b[0] == 0x01);
        CHECK(b[1] == 0x02);
        CHECK(b[2] == 0x03);
        CHECK(b[3] == 0x04);
        CHECK(b[4] == 0x05);
    }

    TEST_CASE("AppendVoidBytes") {
        Bytes b = {0x10};
        u8 more[] = {0x20, 0x30};
        b.append(static_cast<void *>(more), 2);

        CHECK(b.size() == 3);
        CHECK(b[0] == 0x10);
        CHECK(b[1] == 0x20);
        CHECK(b[2] == 0x30);
    }

    TEST_CASE("AppendBytes") {
        Bytes b1 = {0x01, 0x02};
        Bytes b2 = {0x03, 0x04, 0x05};
        b1.append(b2);

        CHECK(b1.size() == 5);
        CHECK(b1[0] == 0x01);
        CHECK(b1[2] == 0x03);
        CHECK(b1[4] == 0x05);
    }

    TEST_CASE("InsertSingle") {
        Bytes b = {0x01, 0x02, 0x03};
        b.insert(b.begin() + 1, 0xFF);

        CHECK(b.size() == 4);
        CHECK(b[0] == 0x01);
        CHECK(b[1] == 0xFF);
        CHECK(b[2] == 0x02);
        CHECK(b[3] == 0x03);
    }

    TEST_CASE("EraseSingle") {
        Bytes b = {0x01, 0x02, 0x03, 0x04};
        b.erase(b.begin() + 1);

        CHECK(b.size() == 3);
        CHECK(b[0] == 0x01);
        CHECK(b[1] == 0x03);
        CHECK(b[2] == 0x04);
    }

    TEST_CASE("Clear") {
        Bytes b(10);
        b[5] = 0xFF;
        CHECK(b.size() == 10);

        b.clear();
        CHECK(b.size() == 0);
        CHECK(b.empty());
    }

    // ========================================================================
    // Byte Operations
    // ========================================================================

    TEST_CASE("Zero") {
        Bytes b = {0x01, 0x02, 0x03, 0x04};
        b.zero();

        CHECK(b.size() == 4);
        for (size_t i = 0; i < b.size(); ++i) {
            CHECK(b[i] == 0x00);
        }
    }

    TEST_CASE("Fill") {
        Bytes b(10);
        b.fill(0xAB);

        for (size_t i = 0; i < b.size(); ++i) {
            CHECK(b[i] == 0xAB);
        }
    }

    // ========================================================================
    // Comparison
    // ========================================================================

    TEST_CASE("Equality") {
        Bytes b1 = {0x01, 0x02, 0x03};
        Bytes b2 = {0x01, 0x02, 0x03};
        Bytes b3 = {0x01, 0x02, 0x04};

        CHECK(b1 == b2);
        CHECK_FALSE(b1 == b3);
        CHECK(b1 != b3);
    }

    TEST_CASE("EqualityEmpty") {
        Bytes b1;
        Bytes b2;
        CHECK(b1 == b2);
    }

    TEST_CASE("LessThan") {
        Bytes b1 = {0x01, 0x02};
        Bytes b2 = {0x01, 0x03};
        CHECK(b1 < b2);
        CHECK_FALSE(b2 < b1);
    }

    TEST_CASE("LessThanPrefix") {
        Bytes b1 = {0x01, 0x02};
        Bytes b2 = {0x01, 0x02, 0x03};
        CHECK(b1 < b2);
    }

    // ========================================================================
    // Search Operations
    // ========================================================================

    TEST_CASE("FindByte") {
        Bytes b = {0x01, 0x02, 0x03, 0x04, 0x05};
        CHECK(b.find(0x03) == 2);
        CHECK(b.find(0x01) == 0);
        CHECK(b.find(0x05) == 4);
        CHECK(b.find(0xFF) == Bytes::npos);
    }

    TEST_CASE("FindByteWithPos") {
        Bytes b = {0x01, 0x02, 0x03, 0x02, 0x04};
        CHECK(b.find(0x02) == 1);
        CHECK(b.find(0x02, 2) == 3);
    }

    TEST_CASE("FindSubsequence") {
        Bytes b = {0x01, 0x02, 0x03, 0x04, 0x05};
        Bytes sub1 = {0x02, 0x03, 0x04};
        Bytes sub2 = {0x03, 0x05};

        CHECK(b.find(sub1) == 1);
        CHECK(b.find(sub2) == Bytes::npos);
    }

    TEST_CASE("RfindByte") {
        Bytes b = {0x01, 0x02, 0x03, 0x02, 0x04};
        CHECK(b.rfind(0x02) == 3);
        CHECK(b.rfind(0x01) == 0);
        CHECK(b.rfind(0xFF) == Bytes::npos);
    }

    TEST_CASE("ContainsByte") {
        Bytes b = {0x01, 0x02, 0x03, 0x04};
        CHECK(b.contains(0x02));
        CHECK_FALSE(b.contains(0xFF));
    }

    TEST_CASE("ContainsSubsequence") {
        Bytes b = {0x01, 0x02, 0x03, 0x04};
        Bytes sub1 = {0x02, 0x03};
        Bytes sub2 = {0x03, 0x05};

        CHECK(b.contains(sub1));
        CHECK_FALSE(b.contains(sub2));
    }

    TEST_CASE("StartsWithByte") {
        Bytes b = {0x01, 0x02, 0x03};
        CHECK(b.starts_with(0x01));
        CHECK_FALSE(b.starts_with(0x02));
    }

    TEST_CASE("StartsWithSubsequence") {
        Bytes b = {0x01, 0x02, 0x03, 0x04};
        Bytes prefix1 = {0x01, 0x02, 0x03};
        Bytes prefix2 = {0x02, 0x03, 0x04};

        CHECK(b.starts_with(prefix1));
        CHECK_FALSE(b.starts_with(prefix2));
    }

    TEST_CASE("EndsWithByte") {
        Bytes b = {0x01, 0x02, 0x03};
        CHECK(b.ends_with(0x03));
        CHECK_FALSE(b.ends_with(0x02));
    }

    TEST_CASE("EndsWithSubsequence") {
        Bytes b = {0x01, 0x02, 0x03, 0x04};
        Bytes suffix1 = {0x02, 0x03, 0x04};
        Bytes suffix2 = {0x01, 0x02, 0x03};

        CHECK(b.ends_with(suffix1));
        CHECK_FALSE(b.ends_with(suffix2));
    }

    // ========================================================================
    // Subbytes
    // ========================================================================

    TEST_CASE("Substr") {
        Bytes b = {0x01, 0x02, 0x03, 0x04, 0x05};
        Bytes sub = b.substr(1, 3);

        CHECK(sub.size() == 3);
        CHECK(sub[0] == 0x02);
        CHECK(sub[1] == 0x03);
        CHECK(sub[2] == 0x04);
    }

    TEST_CASE("SubstrOutOfBounds") {
        Bytes b = {0x01, 0x02, 0x03};
        Bytes sub = b.substr(10);
        CHECK(sub.empty());
    }

    // ========================================================================
    // Concatenation
    // ========================================================================

    TEST_CASE("Concatenation") {
        Bytes b1 = {0x01, 0x02};
        Bytes b2 = {0x03, 0x04};
        Bytes result = b1 + b2;

        CHECK(result.size() == 4);
        CHECK(result[0] == 0x01);
        CHECK(result[1] == 0x02);
        CHECK(result[2] == 0x03);
        CHECK(result[3] == 0x04);
    }

    TEST_CASE("ConcatenationWithByte") {
        Bytes b1 = {0x01, 0x02};
        Bytes result = b1 + 0x03;

        CHECK(result.size() == 3);
        CHECK(result[0] == 0x01);
        CHECK(result[1] == 0x02);
        CHECK(result[2] == 0x03);
    }

    // ========================================================================
    // Copy and Move
    // ========================================================================

    TEST_CASE("CopyConstruction") {
        Bytes b1 = {0x01, 0x02, 0x03};
        Bytes b2 = b1;

        CHECK(b2.size() == b1.size());
        CHECK(b2[0] == 0x01);
        CHECK(b2[1] == 0x02);
        CHECK(b2[2] == 0x03);
    }

    TEST_CASE("MoveConstruction") {
        Bytes b1 = {0x01, 0x02, 0x03};
        Bytes b2 = std::move(b1);

        CHECK(b2.size() == 3);
        CHECK(b2[0] == 0x01);
        CHECK(b2[1] == 0x02);
        CHECK(b2[2] == 0x03);
    }

    TEST_CASE("CopyAssignment") {
        Bytes b1 = {0x01, 0x02, 0x03};
        Bytes b2;
        b2 = b1;

        CHECK(b2 == b1);
    }

    TEST_CASE("MoveAssignment") {
        Bytes b1 = {0x01, 0x02, 0x03};
        Bytes b2;
        b2 = std::move(b1);

        CHECK(b2.size() == 3);
        CHECK(b2[0] == 0x01);
    }

    TEST_CASE("InitializerListAssignment") {
        Bytes b;
        b = {0x01, 0x02, 0x03};

        CHECK(b.size() == 3);
        CHECK(b[0] == 0x01);
        CHECK(b[1] == 0x02);
        CHECK(b[2] == 0x03);
    }

    // ========================================================================
    // Swap
    // ========================================================================

    TEST_CASE("Swap") {
        Bytes b1 = {0x01, 0x02};
        Bytes b2 = {0x03, 0x04};

        b1.swap(b2);

        CHECK(b1[0] == 0x03);
        CHECK(b1[1] == 0x04);
        CHECK(b2[0] == 0x01);
        CHECK(b2[1] == 0x02);
    }

    // ========================================================================
    // Serialization
    // ========================================================================

    TEST_CASE("Members") {
        Bytes b(10);
        b[0] = 0x01;
        b[5] = 0xFF;
        b[9] = 0x55;

        auto [data] = b.members();
        CHECK(data.size() == 10);
        CHECK(data[0] == 0x01);
        CHECK(data[5] == 0xFF);
        CHECK(data[9] == 0x55);
    }

    // ========================================================================
    // Edge Cases
    // ========================================================================

    TEST_CASE("LargeBytes") {
        Bytes b(10000);
        CHECK(b.size() == 10000);

        b[5000] = 0xFF;
        CHECK(b[5000] == 0xFF);

        b[9999] = 0xAA;
        CHECK(b[9999] == 0xAA);
    }

    TEST_CASE("SingleByte") {
        Bytes b(1);
        CHECK(b.size() == 1);
        CHECK(b[0] == 0x00);

        b[0] = 0xFF;
        CHECK(b[0] == 0xFF);
    }

    TEST_CASE("PushBackManyBytes") {
        Bytes b;
        for (int i = 0; i < 1000; ++i) {
            b.push_back(static_cast<u8>(i & 0xFF));
        }

        CHECK(b.size() == 1000);
        CHECK(b[0] == 0x00);
        CHECK(b[255] == 0xFF);
        CHECK(b[256] == 0x00);
    }

    TEST_CASE("ClearAndReuse") {
        Bytes b(100);
        b[50] = 0xFF;
        CHECK(b.size() == 100);

        b.clear();
        CHECK(b.empty());

        b.push_back(0xAA);
        CHECK(b.size() == 1);
        CHECK(b[0] == 0xAA);
    }

    TEST_CASE("MemcpyComparison") {
        u8 raw1[100], raw2[100];
        std::memset(raw1, 0xAA, 100);
        std::memset(raw2, 0xAA, 100);

        Bytes b1(raw1, 100);
        Bytes b2(raw2, 100);

        CHECK(b1 == b2);
        CHECK(b1.size() == 100);

        raw2[50] = 0xBB;
        Bytes b3(raw2, 100);
        CHECK_FALSE(b1 == b3);
    }

    TEST_CASE("ZeroFilledComparison") {
        Bytes b1(100, 0x00);
        Bytes b2(100, 0x00);
        Bytes b3(100, 0x01);

        CHECK(b1 == b2);
        CHECK_FALSE(b1 == b3);
    }
}
