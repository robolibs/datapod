#include <iostream>
#include <string_view>

#include "datagram/datagram.hpp"

using namespace datagram;

// Test fixture helpers
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

// ==================== String Tests ====================

TEST(test_string_default_constructor) {
    BasicString<> str;
    ASSERT_EQ(str.size(), 0);
    ASSERT_EQ(str.empty(), true);
    ASSERT_EQ(str[0], '\0');
}

TEST(test_string_sso_small) {
    // Test SSO with small string (under 23 bytes)
    BasicString<> str("Hello, World!");
    ASSERT_EQ(str.size(), 13);
    ASSERT_EQ(str.empty(), false);
    ASSERT_EQ(std::string_view(str.data(), str.size()), "Hello, World!");
}

TEST(test_string_sso_boundary) {
    // Test SSO boundary (exactly 23 bytes)
    std::string_view boundary = "12345678901234567890123"; // 23 chars
    BasicString<> str(boundary);
    ASSERT_EQ(str.size(), 23);
    ASSERT_EQ(std::string_view(str.data(), str.size()), boundary);
}

TEST(test_string_heap_allocation) {
    // Test heap allocation with large string (over 23 bytes)
    std::string_view large = "This is a very long string that exceeds the SSO buffer size";
    BasicString<> str(large);
    ASSERT_EQ(str.size(), large.size());
    ASSERT_EQ(std::string_view(str.data(), str.size()), large);
}

TEST(test_string_copy_constructor) {
    BasicString<> str1("Copy me!");
    BasicString<> str2(str1);
    ASSERT_EQ(str1.size(), str2.size());
    ASSERT_EQ(std::string_view(str1.data(), str1.size()), std::string_view(str2.data(), str2.size()));
}

TEST(test_string_move_constructor) {
    BasicString<> str1("Move me!");
    auto const orig_size = str1.size();
    BasicString<> str2(std::move(str1));
    ASSERT_EQ(str2.size(), orig_size);
    ASSERT_EQ(std::string_view(str2.data(), str2.size()), "Move me!");
}

TEST(test_string_assignment) {
    BasicString<> str1("Original");
    BasicString<> str2("Different");
    str1 = str2;
    ASSERT_EQ(std::string_view(str1.data(), str1.size()), "Different");
}

TEST(test_string_comparison) {
    BasicString<> str1("abc");
    BasicString<> str2("abc");
    BasicString<> str3("xyz");

    ASSERT(str1 == str2);
    ASSERT(str1 != str3);
    ASSERT(str1 < str3);
}

// ==================== Vector Tests ====================

TEST(test_vector_default_constructor) {
    BasicVector<int> vec;
    ASSERT_EQ(vec.size(), 0);
    ASSERT_EQ(vec.empty(), true);
}

TEST(test_vector_push_back) {
    BasicVector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    ASSERT_EQ(vec.size(), 3);
    ASSERT_EQ(vec[0], 1);
    ASSERT_EQ(vec[1], 2);
    ASSERT_EQ(vec[2], 3);
}

TEST(test_vector_resize) {
    BasicVector<int> vec;
    vec.resize(5, 42);
    ASSERT_EQ(vec.size(), 5);
    for (std::size_t i = 0; i < vec.size(); ++i) {
        ASSERT_EQ(vec[i], 42);
    }

    vec.resize(3);
    ASSERT_EQ(vec.size(), 3);
}

TEST(test_vector_reserve) {
    BasicVector<int> vec;
    vec.reserve(100);
    ASSERT_EQ(vec.capacity() >= 100, true);
    ASSERT_EQ(vec.size(), 0);
}

TEST(test_vector_copy_constructor) {
    BasicVector<int> vec1;
    vec1.push_back(1);
    vec1.push_back(2);
    vec1.push_back(3);

    BasicVector<int> vec2(vec1);
    ASSERT_EQ(vec2.size(), 3);
    ASSERT_EQ(vec2[0], 1);
    ASSERT_EQ(vec2[1], 2);
    ASSERT_EQ(vec2[2], 3);
}

TEST(test_vector_move_constructor) {
    BasicVector<int> vec1;
    vec1.push_back(1);
    vec1.push_back(2);

    BasicVector<int> vec2(std::move(vec1));
    ASSERT_EQ(vec2.size(), 2);
    ASSERT_EQ(vec2[0], 1);
    ASSERT_EQ(vec2[1], 2);
}

TEST(test_vector_clear) {
    BasicVector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.clear();

    ASSERT_EQ(vec.size(), 0);
    ASSERT_EQ(vec.empty(), true);
}

TEST(test_vector_iteration) {
    BasicVector<int> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    int sum = 0;
    for (auto const &val : vec) {
        sum += val;
    }
    ASSERT_EQ(sum, 60);
}

TEST(test_vector_of_strings) {
    BasicVector<BasicString<>> vec;
    vec.push_back("Hello");
    vec.push_back("World");

    ASSERT_EQ(vec.size(), 2);
    ASSERT_EQ(std::string_view(vec[0].data(), vec[0].size()), "Hello");
    ASSERT_EQ(std::string_view(vec[1].data(), vec[1].size()), "World");
}

// ==================== Optional Tests ====================

TEST(test_optional_default_constructor) {
    Optional<int> opt;
    ASSERT_EQ(opt.has_value(), false);
}

TEST(test_optional_value_constructor) {
    Optional<int> opt(42);
    ASSERT_EQ(opt.has_value(), true);
    ASSERT_EQ(opt.value(), 42);
}

TEST(test_optional_reset) {
    Optional<int> opt(42);
    ASSERT_EQ(opt.has_value(), true);

    opt.reset();
    ASSERT_EQ(opt.has_value(), false);
}

TEST(test_optional_emplace) {
    Optional<int> opt;
    opt.emplace(123);

    ASSERT_EQ(opt.has_value(), true);
    ASSERT_EQ(opt.value(), 123);
}

TEST(test_optional_value_or) {
    Optional<int> opt1(42);
    Optional<int> opt2;

    ASSERT_EQ(opt1.value_or(0), 42);
    ASSERT_EQ(opt2.value_or(99), 99);
}

TEST(test_optional_copy_constructor) {
    Optional<int> opt1(42);
    Optional<int> opt2(opt1);

    ASSERT_EQ(opt2.has_value(), true);
    ASSERT_EQ(opt2.value(), 42);
}

TEST(test_optional_move_constructor) {
    Optional<int> opt1(42);
    Optional<int> opt2(std::move(opt1));

    ASSERT_EQ(opt2.has_value(), true);
    ASSERT_EQ(opt2.value(), 42);
}

TEST(test_optional_with_string) {
    Optional<BasicString<>> opt("Hello");
    ASSERT_EQ(opt.has_value(), true);
    ASSERT_EQ(std::string_view(opt.value().data(), opt.value().size()), "Hello");
}

TEST(test_optional_bool_conversion) {
    Optional<int> opt1(42);
    Optional<int> opt2;

    ASSERT(opt1);
    ASSERT(!opt2);
}

// ==================== Array Tests ====================

TEST(test_array_construction) {
    Array<int, 5> arr = {1, 2, 3, 4, 5};
    ASSERT_EQ(arr.size(), 5);
    ASSERT_EQ(arr[0], 1);
    ASSERT_EQ(arr[4], 5);
}

TEST(test_array_iteration) {
    Array<int, 3> arr = {10, 20, 30};
    int sum = 0;
    for (auto val : arr) {
        sum += val;
    }
    ASSERT_EQ(sum, 60);
}

TEST(test_array_front_back) {
    Array<int, 4> arr = {1, 2, 3, 4};
    ASSERT_EQ(arr.front(), 1);
    ASSERT_EQ(arr.back(), 4);
}

// ==================== Pair Tests ====================

TEST(test_pair_construction) {
    Pair<int, BasicString<>> p(42, "Hello");
    ASSERT_EQ(p.first, 42);
    ASSERT_EQ(std::string_view(p.second.data(), p.second.size()), "Hello");
}

TEST(test_pair_copy) {
    Pair<int, int> p1(1, 2);
    Pair<int, int> p2(p1);
    ASSERT_EQ(p2.first, 1);
    ASSERT_EQ(p2.second, 2);
}

// ==================== UniquePtr Tests ====================

TEST(test_unique_ptr_default) {
    UniquePtr<int> ptr;
    ASSERT_EQ(ptr.get(), nullptr);
    ASSERT(!ptr);
}

TEST(test_unique_ptr_construction) {
    UniquePtr<int> ptr(new int(42));
    ASSERT(ptr);
    ASSERT_EQ(*ptr, 42);
}

TEST(test_unique_ptr_move) {
    UniquePtr<int> ptr1(new int(42));
    UniquePtr<int> ptr2(std::move(ptr1));

    ASSERT(!ptr1);
    ASSERT(ptr2);
    ASSERT_EQ(*ptr2, 42);
}

TEST(test_unique_ptr_reset) {
    UniquePtr<int> ptr(new int(42));
    ptr.reset(new int(99));
    ASSERT_EQ(*ptr, 99);

    ptr.reset();
    ASSERT(!ptr);
}

TEST(test_unique_ptr_release) {
    UniquePtr<int> ptr(new int(42));
    int *raw = ptr.release();
    ASSERT(!ptr);
    ASSERT_EQ(*raw, 42);
    delete raw; // manual cleanup
}

// ==================== Hashing Tests ====================

TEST(test_hash_fundamental_types) {
    auto h1 = hash_value(42);
    auto h2 = hash_value(42);
    auto h3 = hash_value(43);

    ASSERT_EQ(h1, h2);
    ASSERT(h1 != h3);
}

TEST(test_hash_string) {
    BasicString<> str1("Hello");
    BasicString<> str2("Hello");
    BasicString<> str3("World");

    auto h1 = hash_value(str1);
    auto h2 = hash_value(str2);
    auto h3 = hash_value(str3);

    ASSERT_EQ(h1, h2);
    ASSERT(h1 != h3);
}

TEST(test_hash_vector) {
    BasicVector<int> vec1;
    vec1.push_back(1);
    vec1.push_back(2);
    vec1.push_back(3);

    BasicVector<int> vec2;
    vec2.push_back(1);
    vec2.push_back(2);
    vec2.push_back(3);

    BasicVector<int> vec3;
    vec3.push_back(1);
    vec3.push_back(2);

    auto h1 = hash_value(vec1);
    auto h2 = hash_value(vec2);
    auto h3 = hash_value(vec3);

    ASSERT_EQ(h1, h2);
    ASSERT(h1 != h3);
}

TEST(test_hash_optional) {
    Optional<int> opt1(42);
    Optional<int> opt2(42);
    Optional<int> opt3(99);
    Optional<int> opt4; // empty

    auto h1 = hash_value(opt1);
    auto h2 = hash_value(opt2);
    auto h3 = hash_value(opt3);
    auto h4 = hash_value(opt4);

    ASSERT_EQ(h1, h2);
    ASSERT(h1 != h3);
    ASSERT(h1 != h4);
}

TEST(test_hash_array) {
    Array<int, 3> arr1 = {1, 2, 3};
    Array<int, 3> arr2 = {1, 2, 3};
    Array<int, 3> arr3 = {4, 5, 6};

    auto h1 = hash_value(arr1);
    auto h2 = hash_value(arr2);
    auto h3 = hash_value(arr3);

    ASSERT_EQ(h1, h2);
    ASSERT(h1 != h3);
}

TEST(test_hash_pair) {
    Pair<int, int> p1(1, 2);
    Pair<int, int> p2(1, 2);
    Pair<int, int> p3(2, 1);

    auto h1 = hash_value(p1);
    auto h2 = hash_value(p2);
    auto h3 = hash_value(p3);

    ASSERT_EQ(h1, h2);
    ASSERT(h1 != h3);
}

TEST(test_hash_aggregate_struct) {
    struct Point {
        int x;
        int y;
    };

    Point p1{10, 20};
    Point p2{10, 20};
    Point p3{20, 10};

    auto h1 = hash_value(p1);
    auto h2 = hash_value(p2);
    auto h3 = hash_value(p3);

    ASSERT_EQ(h1, h2);
    ASSERT(h1 != h3);
}

TEST(test_hash_nested_struct) {
    struct Inner {
        int value;
    };

    struct Outer {
        Inner inner;
        int extra;
    };

    Outer o1{{42}, 99};
    Outer o2{{42}, 99};
    Outer o3{{42}, 100};

    auto h1 = hash_value(o1);
    auto h2 = hash_value(o2);
    auto h3 = hash_value(o3);

    ASSERT_EQ(h1, h2);
    ASSERT(h1 != h3);
}

int main() {
    std::cout << "\n=== Phase 5 Container Tests ===" << std::endl;
    std::cout << "\nAll tests passed! ✓\n" << std::endl;
    return 0;
}
