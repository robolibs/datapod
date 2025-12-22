#include <cassert>
#include <iostream>
#include <string>

#include "datapod/reflection/arity.hpp"
#include "datapod/reflection/comparable.hpp"
#include "datapod/reflection/for_each_field.hpp"
#include "datapod/reflection/to_tuple.hpp"

// Test arity detection
void test_arity() {
    std::cout << "Testing arity detection... ";

    // Single field
    struct One {
        int x;
    };
    static_assert(datapod::arity_v<One> == 1);

    // Two fields
    struct Two {
        int x;
        double y;
    };
    static_assert(datapod::arity_v<Two> == 2);

    // Three fields
    struct Three {
        int x;
        double y;
        int z;
    };
    static_assert(datapod::arity_v<Three> == 3);

    // Five fields
    struct Five {
        int a;
        int b;
        int c;
        int d;
        int e;
    };
    static_assert(datapod::arity_v<Five> == 5);

    std::cout << "PASSED\n";
}

// Test to_tuple conversion
void test_to_tuple() {
    std::cout << "Testing to_tuple... ";

    struct Point {
        int x;
        int y;
    };

    Point p{10, 20};
    auto tup = datapod::to_tuple(p);

    assert(std::get<0>(tup) == 10);
    assert(std::get<1>(tup) == 20);

    std::cout << "PASSED\n";
}

// Test for_each_field
void test_for_each_field() {
    std::cout << "Testing for_each_field... ";

    struct Point {
        int x;
        int y;
    };

    Point p{10, 20};

    // Count fields
    int count = 0;
    datapod::for_each_field(p, [&count](auto &&) { ++count; });
    assert(count == 2);

    // Sum all fields
    int sum = 0;
    datapod::for_each_field(p, [&sum](auto &&field) { sum += field; });
    assert(sum == 30);

    std::cout << "PASSED\n";
}

// Test for_each_field_indexed
void test_for_each_field_indexed() {
    std::cout << "Testing for_each_field_indexed... ";

    struct Data {
        int a;
        int b;
        int c;
    };

    Data d{10, 20, 30};

    int indices_sum = 0;
    int values_sum = 0;

    datapod::for_each_field_indexed(d, [&](auto &&field, auto index) {
        indices_sum += index;
        values_sum += field;
    });

    assert(indices_sum == 0 + 1 + 2); // 3
    assert(values_sum == 60);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== Datagram Reflection Tests ===\n\n";

    test_arity();
    test_to_tuple();
    test_for_each_field();
    test_for_each_field_indexed();

    std::cout << "\n=== All Reflection Tests PASSED ===\n";
    return 0;
}
