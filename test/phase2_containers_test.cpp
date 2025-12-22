#include <iostream>
#include <string>
#include <vector>

#include "datapod/adapters/bitset.hpp"
#include "datapod/sequential/bitvec.hpp"
#include "datapod/sequential/cstring.hpp"
#include "datapod/matrix/flat_matrix.hpp"
#include "datapod/matrix/vecvec.hpp"

using namespace datapod;

#define TEST(name)                                                                                                     \
    void name();                                                                                                       \
    struct name##_runner {                                                                                             \
        name##_runner() {                                                                                              \
            std::cout << "Running " #name "..." << std::endl;                                                          \
            name();                                                                                                    \
            std::cout << "  ✓ " #name " passed" << std::endl;                                                          \
        }                                                                                                              \
    } name##_instance;                                                                                                 \
    void name()

#define ASSERT(cond)                                                                                                   \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::cerr << "Assertion failed: " #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl;             \
            std::exit(1);                                                                                              \
        }                                                                                                              \
    } while (0)

#define ASSERT_EQ(a, b)                                                                                                \
    do {                                                                                                               \
        if ((a) != (b)) {                                                                                              \
            std::cerr << "Assertion failed: " #a " == " #b << " at " << __FILE__ << ":" << __LINE__ << std::endl;      \
            std::exit(1);                                                                                              \
        }                                                                                                              \
    } while (0)

// ==================== Bitset Tests ====================

TEST(test_bitset_default) {
    Bitset<64> bs;
    ASSERT_EQ(bs.count(), 0);
    ASSERT_EQ(bs.none(), true);
    ASSERT_EQ(bs.any(), false);
}

TEST(test_bitset_set) {
    Bitset<64> bs;
    bs.set(5);
    ASSERT_EQ(bs.test(5), true);
    ASSERT_EQ(bs.count(), 1);
    bs.set(5, false);
    ASSERT_EQ(bs.test(5), false);
    ASSERT_EQ(bs.count(), 0);
}

TEST(test_bitset_bitwise_and) {
    Bitset<64> a, b;
    a.set(1);
    a.set(2);
    b.set(2);
    b.set(3);
    auto c = a & b;
    ASSERT_EQ(c.test(1), false);
    ASSERT_EQ(c.test(2), true);
    ASSERT_EQ(c.test(3), false);
    ASSERT_EQ(c.count(), 1);
}

TEST(test_bitset_for_each_set_bit) {
    Bitset<64> bs;
    bs.set(1);
    bs.set(10);
    bs.set(20);
    std::vector<std::size_t> indices;
    bs.for_each_set_bit([&](auto i) { indices.push_back(i); });
    ASSERT_EQ(indices.size(), 3);
    ASSERT_EQ(indices[0], 1);
    ASSERT_EQ(indices[1], 10);
    ASSERT_EQ(indices[2], 20);
}

// ==================== Bitvec Tests ====================

TEST(test_bitvec_basic) {
    using BV = BasicBitvec<Vector<std::uint64_t>>;
    BV bv;
    bv.resize(100);
    ASSERT_EQ(bv.size(), 100);
    bv.set(42, true);
    ASSERT_EQ(bv.test(42), true);
    ASSERT_EQ(bv.count(), 1);
}

// ==================== FlatMatrix Tests ====================

TEST(test_flat_matrix_basic) {
    FlatMatrix<int> m;
    m.resize(3, 4);
    m(1, 2) = 42;
    ASSERT_EQ(m(1, 2), 42);
    ASSERT_EQ(m[1][2], 42);
}

TEST(test_flat_matrix_reset) {
    FlatMatrix<int> m;
    m.resize(2, 2);
    m.reset(99);
    ASSERT_EQ(m(0, 0), 99);
    ASSERT_EQ(m(0, 1), 99);
    ASSERT_EQ(m(1, 0), 99);
    ASSERT_EQ(m(1, 1), 99);
}

// ==================== Vecvec Tests ====================

TEST(test_vecvec_default) {
    Vecvec<std::size_t, int> vv;
    ASSERT_EQ(vv.empty(), true);
    ASSERT_EQ(vv.size(), 0);
}

TEST(test_vecvec_emplace_back) {
    Vecvec<std::size_t, int> vv;
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = {4, 5};
    vv.emplace_back(v1);
    vv.emplace_back(v2);

    ASSERT_EQ(vv.size(), 2);
    ASSERT_EQ(vv[0].size(), 3);
    ASSERT_EQ(vv[1].size(), 2);
    ASSERT_EQ(vv[0][0], 1);
    ASSERT_EQ(vv[0][1], 2);
    ASSERT_EQ(vv[0][2], 3);
    ASSERT_EQ(vv[1][0], 4);
    ASSERT_EQ(vv[1][1], 5);
}

TEST(test_vecvec_add_back_sized) {
    Vecvec<std::size_t, int> vv;
    auto bucket = vv.add_back_sized(5);
    ASSERT_EQ(vv.size(), 1);
    ASSERT_EQ(bucket.size(), 5);
    bucket[0] = 10;
    bucket[4] = 50;
    ASSERT_EQ(vv[0][0], 10);
    ASSERT_EQ(vv[0][4], 50);
}

TEST(test_vecvec_iteration) {
    Vecvec<std::size_t, int> vv;
    vv.emplace_back(std::vector<int>{1, 2, 3});
    vv.emplace_back(std::vector<int>{4, 5});

    int sum = 0;
    for (auto bucket : vv) {
        for (auto val : bucket) {
            sum += val;
        }
    }
    ASSERT_EQ(sum, 15); // 1+2+3+4+5
}

// ==================== Cstring Tests ====================

TEST(test_cstring_default) {
    Cstring str;
    ASSERT_EQ(str.empty(), true);
    ASSERT_EQ(str.size(), 0);
}

TEST(test_cstring_sso_short) {
    Cstring str("Hello");
    ASSERT_EQ(str.is_short(), true);
    ASSERT_EQ(str.size(), 5);
    ASSERT_EQ(str.view(), "Hello");
    ASSERT_EQ(std::string(str.c_str()), "Hello");
}

TEST(test_cstring_heap_long) {
    Cstring str("1234567890123456"); // 16 chars - exceeds SSO
    ASSERT_EQ(str.is_short(), false);
    ASSERT_EQ(str.is_owning(), true);
    ASSERT_EQ(str.size(), 16);
    ASSERT_EQ(str.view(), "1234567890123456");
}

TEST(test_cstring_set_owning) {
    Cstring str;
    str.set_owning("World");
    ASSERT_EQ(str.is_owning(), true);
    ASSERT_EQ(str.size(), 5);
    ASSERT_EQ(str.view(), "World");
}

TEST(test_cstring_copy) {
    Cstring str1("Test");
    Cstring str2(str1);
    ASSERT_EQ(str2.view(), "Test");
    ASSERT_EQ(str2.size(), 4);
}

TEST(test_cstring_comparison) {
    Cstring str1("Apple");
    Cstring str2("Banana");
    Cstring str3("Apple");

    ASSERT_EQ(str1 == str3, true);
    ASSERT_EQ(str1 != str2, true);
    ASSERT_EQ(str1 < str2, true);
    ASSERT_EQ(str2 > str1, true);
}

int main() {
    std::cout << "\n=== Phase 2 Containers Test Suite ===" << std::endl;
    std::cout << "\nAll tests completed successfully!" << std::endl;
    return 0;
}
