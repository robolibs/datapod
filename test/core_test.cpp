#include <cassert>
#include <iostream>
#include <string>

#include "bitcon/core/decay.hpp"
#include "bitcon/core/exception.hpp"
#include "bitcon/core/hash.hpp"
#include "bitcon/core/mode.hpp"
#include "bitcon/core/offset_t.hpp"
#include "bitcon/core/type_traits.hpp"
#include "bitcon/core/verify.hpp"

// Test exception handling
void test_exception() {
    std::cout << "Testing exception handling... ";

    bool caught = false;
    try {
        bitcon::throw_exception(bitcon::BitconException{"test error"});
    } catch (const bitcon::BitconException &e) {
        caught = true;
        assert(std::string(e.what()) == "test error");
    }
    assert(caught);

    std::cout << "PASSED\n";
}

// Test verify functions
void test_verify() {
    std::cout << "Testing verify... ";

    // Should not throw
    bitcon::verify(true, "this should not throw");
    bitcon::verify_str(true, "this should not throw either");

    // Should throw
    bool caught = false;
    try {
        bitcon::verify(false, "expected failure");
    } catch (const bitcon::BitconException &e) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        bitcon::verify_str(false, std::string("expected failure with string"));
    } catch (const bitcon::BitconException &e) {
        caught = true;
    }
    assert(caught);

    std::cout << "PASSED\n";
}

// Test offset_t constants
void test_offset_t() {
    std::cout << "Testing offset_t... ";

    // Check that NULLPTR_OFFSET is the minimum value
    assert(bitcon::NULLPTR_OFFSET == std::numeric_limits<bitcon::offset_t>::min());

    // Check that DANGLING is NULLPTR_OFFSET + 1
    assert(bitcon::DANGLING == bitcon::NULLPTR_OFFSET + 1);

    // Check that offset_t is intptr_t
    static_assert(std::is_same_v<bitcon::offset_t, std::intptr_t>);

    std::cout << "PASSED\n";
}

// Test decay_t
void test_decay() {
    std::cout << "Testing decay_t... ";

    // Basic decay
    static_assert(std::is_same_v<bitcon::decay_t<int &>, int>);
    static_assert(std::is_same_v<bitcon::decay_t<const int &>, int>);
    static_assert(std::is_same_v<bitcon::decay_t<int &&>, int>);

    // Reference wrapper unwrapping
    int x = 42;
    std::reference_wrapper<int> ref(x);
    static_assert(std::is_same_v<bitcon::decay_t<decltype(ref)>, int>);

    std::cout << "PASSED\n";
}

// Test Mode enum
void test_mode() {
    std::cout << "Testing Mode enum... ";

    using bitcon::is_mode_disabled;
    using bitcon::is_mode_enabled;
    using bitcon::Mode;

    // Test combining modes
    auto combined = Mode::WITH_VERSION | Mode::WITH_INTEGRITY;
    assert(is_mode_enabled(combined, Mode::WITH_VERSION));
    assert(is_mode_enabled(combined, Mode::WITH_INTEGRITY));
    assert(is_mode_disabled(combined, Mode::UNCHECKED));

    // Test NONE mode
    assert(is_mode_disabled(Mode::NONE, Mode::WITH_VERSION));

    std::cout << "PASSED\n";
}

// Test type traits
void test_type_traits() {
    std::cout << "Testing type_traits... ";

    // is_char_array_v
    static_assert(bitcon::is_char_array_v<char[10]>);
    static_assert(bitcon::is_char_array_v<const char[10]>);
    static_assert(!bitcon::is_char_array_v<int[10]>);
    static_assert(!bitcon::is_char_array_v<std::string>);

    // is_iterable_v
    static_assert(bitcon::is_iterable_v<std::string>);
    static_assert(bitcon::is_iterable_v<std::vector<int>>);
    static_assert(!bitcon::is_iterable_v<int>);

    std::cout << "PASSED\n";
}

// Test hash functions
void test_hash() {
    std::cout << "Testing hash... ";

    using bitcon::BASE_HASH;
    using bitcon::hash;
    using bitcon::hash_combine;

    // Hash consistency - same input should give same output
    auto h1 = hash("hello");
    auto h2 = hash("hello");
    assert(h1 == h2);

    // Different inputs should (very likely) give different outputs
    auto h3 = hash("world");
    assert(h1 != h3);

    // Empty string
    auto h4 = hash("");
    assert(h4 == BASE_HASH);

    // hash_combine
    auto hc1 = hash_combine(BASE_HASH, 1, 2, 3);
    auto hc2 = hash_combine(BASE_HASH, 1, 2, 3);
    assert(hc1 == hc2);

    auto hc3 = hash_combine(BASE_HASH, 3, 2, 1);
    assert(hc1 != hc3); // Order matters

    // String view hash
    std::string_view sv = "test string";
    auto hsv = hash(sv);
    assert(hsv != BASE_HASH);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== Datagram Core Tests ===\n\n";

    test_exception();
    test_verify();
    test_offset_t();
    test_decay();
    test_mode();
    test_type_traits();
    test_hash();

    std::cout << "\n=== All Core Tests PASSED ===\n";
    return 0;
}
