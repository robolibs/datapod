#include <iostream>
#include "datapod/datapod.hpp"

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
            std::cerr << "Assertion failed: " #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl;            \
            std::exit(1);                                                                                              \
        }                                                                                                              \
    } while (0)

#define ASSERT_EQ(a, b)                                                                                                \
    do {                                                                                                               \
        if ((a) != (b)) {                                                                                              \
            std::cerr << "Assertion failed: " #a " == " #b << " at " << __FILE__ << ":" << __LINE__ << std::endl;     \
            std::exit(1);                                                                                              \
        }                                                                                                              \
    } while (0)

// ==================== Variant Tests ====================

TEST(test_variant_default) {
    Variant<int, double, BasicString<>> v;
    ASSERT_EQ(v.valid(), false);
    auto no_val = Variant<int, double, BasicString<>>::NO_VALUE; ASSERT_EQ(v.index(), no_val);
}

TEST(test_variant_construct_int) {
    Variant<int, double, BasicString<>> v(42);
    ASSERT_EQ(v.valid(), true);
    ASSERT_EQ(v.index(), 0);
    ASSERT_EQ(get<0>(v), 42);
    ASSERT_EQ(get<int>(v), 42);
}

TEST(test_variant_construct_string) {
    Variant<int, double, BasicString<>> v(BasicString<>("hello"));
    ASSERT_EQ(v.valid(), true);
    ASSERT_EQ(v.index(), 2);
    ASSERT_EQ(std::string_view(get<2>(v).data(), get<2>(v).size()), "hello");
}

TEST(test_variant_copy) {
    Variant<int, double> v1(3.14);
    Variant<int, double> v2(v1);
    ASSERT_EQ(v2.index(), 1);
    ASSERT_EQ(get<1>(v2), 3.14);
}

TEST(test_variant_assign) {
    Variant<int, double> v;
    v = 100;
    ASSERT_EQ(v.index(), 0);
    ASSERT_EQ(get<0>(v), 100);
    
    v = 2.718;
    ASSERT_EQ(v.index(), 1);
    ASSERT_EQ(get<1>(v), 2.718);
}

TEST(test_variant_emplace) {
    Variant<int, double, BasicString<>> v;
    v.emplace<BasicString<>>("world");
    ASSERT_EQ(v.index(), 2);
    ASSERT_EQ(std::string_view(get<2>(v).data(), get<2>(v).size()), "world");
}

TEST(test_variant_holds_alternative) {
    Variant<int, double, BasicString<>> v(42);
    ASSERT(holds_alternative<int>(v));
    ASSERT(!holds_alternative<double>(v));
}

TEST(test_variant_apply) {
    Variant<int, double> v(42);
    int result = v.apply([](auto x) { return static_cast<int>(x) * 2; });
    ASSERT_EQ(result, 84);
}

TEST(test_variant_comparison) {
    Variant<int, double> v1(42);
    Variant<int, double> v2(42);
    Variant<int, double> v3(99);
    
    ASSERT(v1 == v2);
    ASSERT(v1 != v3);
    ASSERT(v1 < v3);
}

// ==================== Tuple Tests ====================

TEST(test_tuple_default) {
    Tuple<int, double, BasicString<>> t;
    // Default constructed - values are default-initialized
    ASSERT_EQ(get<0>(t), 0);
}

TEST(test_tuple_construct) {
    Tuple<int, double, BasicString<>> t(42, 3.14, BasicString<>("test"));
    ASSERT_EQ(get<0>(t), 42);
    ASSERT_EQ(get<1>(t), 3.14);
    ASSERT_EQ(std::string_view(get<2>(t).data(), get<2>(t).size()), "test");
}

TEST(test_tuple_copy) {
    Tuple<int, double> t1(10, 2.5);
    Tuple<int, double> t2(t1);
    ASSERT_EQ(get<0>(t2), 10);
    ASSERT_EQ(get<1>(t2), 2.5);
}

TEST(test_tuple_assign) {
    Tuple<int, double> t1(10, 2.5);
    Tuple<int, double> t2;
    t2 = t1;
    ASSERT_EQ(get<0>(t2), 10);
    ASSERT_EQ(get<1>(t2), 2.5);
}

TEST(test_tuple_comparison) {
    Tuple<int, double> t1(10, 2.5);
    Tuple<int, double> t2(10, 2.5);
    Tuple<int, double> t3(20, 3.0);
    
    ASSERT(t1 == t2);
    ASSERT(t1 != t3);
    ASSERT(t1 < t3);
}

TEST(test_tuple_apply) {
    Tuple<int, double> t(10, 2.5);
    double result = apply([](int a, double b) { return a + b; }, t);
    ASSERT_EQ(result, 12.5);
}

int main() {
    std::cout << "\n=== Phase 7 Advanced Container Tests ===" << std::endl;
    std::cout << "\nAll tests passed! ✓\n" << std::endl;
    return 0;
}
