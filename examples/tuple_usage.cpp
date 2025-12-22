#include "datapod/containers/tuple.hpp"
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace datapod;

// Example 1: Basic Tuple Usage
void basic_usage() {
    std::cout << "=== Example 1: Basic Tuple Usage ===\n";

    // Create a tuple
    Tuple<int, double, std::string> t(42, 3.14, std::string("hello"));
    std::cout << "Tuple: (" << get<0>(t) << ", " << get<1>(t) << ", " << get<2>(t) << ")\n";

    // Default construction
    Tuple<int, double> t2;
    std::cout << "Default tuple: (" << get<0>(t2) << ", " << get<1>(t2) << ")\n";

    // Deduction guide
    Tuple t3(true, 'A', 100);
    std::cout << "Deduced tuple: (" << get<0>(t3) << ", " << get<1>(t3) << ", " << get<2>(t3) << ")\n";
    std::cout << "\n";
}

// Example 2: Structured Bindings
void structured_bindings() {
    std::cout << "=== Example 2: Structured Bindings ===\n";

    Tuple<int, double, std::string> t(42, 3.14159, std::string("pi"));

    // Decompose into separate variables
    auto [num, pi, name] = t;
    std::cout << "Decomposed: num=" << num << ", pi=" << pi << ", name=" << name << "\n";

    // Modify via structured binding references
    auto &[n, p, s] = t;
    n = 100;
    p = 2.71828;
    s = "e";
    std::cout << "Modified: (" << get<0>(t) << ", " << get<1>(t) << ", " << get<2>(t) << ")\n";

    // Const structured bindings
    Tuple<int, int> const ct(10, 20);
    auto const &[x, y] = ct;
    std::cout << "Const tuple: x=" << x << ", y=" << y << "\n";
    std::cout << "\n";
}

// Example 3: Member apply() - Function Application
void member_apply() {
    std::cout << "=== Example 3: Member apply() ===\n";

    // Sum of elements
    Tuple<int, int, int> numbers(10, 20, 30);
    auto sum = numbers.apply([](int a, int b, int c) { return a + b + c; });
    std::cout << "Sum: " << sum << "\n";

    // Calculate distance
    Tuple<double, double> point(3.0, 4.0);
    auto distance = point.apply([](double x, double y) { return std::sqrt(x * x + y * y); });
    std::cout << "Distance from origin: " << distance << "\n";

    // String concatenation
    Tuple<std::string, std::string, std::string> words("Hello", " ", "World");
    auto sentence = words.apply([](auto const &a, auto const &b, auto const &c) { return a + b + c; });
    std::cout << "Sentence: " << sentence << "\n";
    std::cout << "\n";
}

// Example 4: Member for_each() - Iteration
void member_for_each() {
    std::cout << "=== Example 4: Member for_each() ===\n";

    // Print all elements
    Tuple<int, double, std::string> t(42, 3.14, std::string("test"));
    std::cout << "Elements: ";
    t.for_each([](auto const &x) { std::cout << x << " "; });
    std::cout << "\n";

    // Modify all elements
    Tuple<int, int, int> nums(1, 2, 3);
    nums.for_each([](auto &x) { x *= 2; });
    std::cout << "Doubled: (" << get<0>(nums) << ", " << get<1>(nums) << ", " << get<2>(nums) << ")\n";

    // Accumulate
    Tuple<int, int, int, int> values(5, 10, 15, 20);
    int total = 0;
    values.for_each([&total](auto const &x) { total += x; });
    std::cout << "Total: " << total << "\n";

    // Collect into vector
    Tuple<int, int, int> items(100, 200, 300);
    std::vector<int> vec;
    items.for_each([&vec](auto const &x) { vec.push_back(x); });
    std::cout << "Vector: [";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0)
            std::cout << ", ";
        std::cout << vec[i];
    }
    std::cout << "]\n";
    std::cout << "\n";
}

// Example 5: Free apply() Function
void free_apply() {
    std::cout << "=== Example 5: Free apply() ===\n";

    Tuple<int, int, int> t(2, 3, 4);

    // Using free apply function
    auto product = apply([](int a, int b, int c) { return a * b * c; }, t);
    std::cout << "Product: " << product << "\n";

    // Apply with lambda that returns different type
    auto description = apply(
        [](int a, int b, int c) {
            return "Values: " + std::to_string(a) + ", " + std::to_string(b) + ", " + std::to_string(c);
        },
        t);
    std::cout << description << "\n";
    std::cout << "\n";
}

// Example 6: Multiple Return Values
Tuple<int, int, int> split_time(int seconds) {
    Tuple<int, int, int> result;
    get<0>(result) = seconds / 3600;
    get<1>(result) = (seconds % 3600) / 60;
    get<2>(result) = seconds % 60;
    return result;
}

void multiple_return_values() {
    std::cout << "=== Example 6: Multiple Return Values ===\n";

    auto [h, m, s] = split_time(3665);
    std::cout << "3665 seconds = " << h << "h " << m << "m " << s << "s\n";

    // Can also use without structured bindings
    auto time = split_time(7384);
    std::cout << "7384 seconds = " << get<0>(time) << "h " << get<1>(time) << "m " << get<2>(time) << "s\n";
    std::cout << "\n";
}

// Example 7: Comparison Operations
void comparison() {
    std::cout << "=== Example 7: Comparison Operations ===\n";

    Tuple<int, int> t1(1, 2);
    Tuple<int, int> t2(1, 2);
    Tuple<int, int> t3(1, 3);
    Tuple<int, int> t4(2, 1);

    std::cout << "t1 == t2: " << (t1 == t2) << "\n";
    std::cout << "t1 != t3: " << (t1 != t3) << "\n";
    std::cout << "t1 < t3: " << (t1 < t3) << "\n";
    std::cout << "t1 < t4: " << (t1 < t4) << "\n";
    std::cout << "t4 > t1: " << (t4 > t1) << "\n";
    std::cout << "\n";
}

// Example 8: Type Traits
void type_traits() {
    std::cout << "=== Example 8: Type Traits ===\n";

    using T = Tuple<int, double, std::string>;

    std::cout << "tuple_size: " << tuple_size_v<T> << "\n";
    std::cout << "is_tuple: " << is_tuple_v<T> << "\n";
    std::cout << "Element 0 is int: " << std::is_same_v<tuple_element_t<0, T>, int> << "\n";
    std::cout << "Element 1 is double: " << std::is_same_v<tuple_element_t<1, T>, double> << "\n";
    std::cout << "Element 2 is string: " << std::is_same_v<tuple_element_t<2, T>, std::string> << "\n";
    std::cout << "\n";
}

// Example 9: Complex Use Case - Data Processing Pipeline
void data_pipeline() {
    std::cout << "=== Example 9: Data Processing Pipeline ===\n";

    // Input data
    Tuple<int, int, int> raw_data(100, 200, 300);

    // Process using for_each (double values and clamp)
    raw_data.for_each([](auto &x) {
        x *= 2;
        if (x > 500)
            x = 500;
    });

    // Extract results
    auto [x, y, z] = raw_data;
    std::cout << "Processed data: (" << x << ", " << y << ", " << z << ")\n";

    // Calculate statistics using apply
    auto sum = raw_data.apply([](int a, int b, int c) { return a + b + c; });
    std::cout << "Sum: " << sum << "\n";

    auto max_val = raw_data.apply([](int a, int b, int c) { return std::max(std::max(a, b), c); });
    std::cout << "Max: " << max_val << "\n";
    std::cout << "\n";
}

// Example 10: Heterogeneous Processing
void heterogeneous_processing() {
    std::cout << "=== Example 10: Heterogeneous Processing ===\n";

    Tuple<int, double, std::string, bool> mixed(42, 3.14, std::string("test"), true);

    // Count elements using for_each
    int count = 0;
    mixed.for_each([&count](auto const &) { ++count; });
    std::cout << "Element count: " << count << "\n";

    // Create description using apply
    auto desc = mixed.apply([](auto const &a, auto const &b, auto const &c, auto const &d) {
        return "Mixed tuple: " + std::to_string(a) + ", " + std::to_string(b) + ", " + c + ", " + std::to_string(d);
    });
    std::cout << desc << "\n";
    std::cout << "\n";
}

int main() {
    std::cout << "Datapod Tuple Usage Examples\n";
    std::cout << "=============================\n\n";

    basic_usage();
    structured_bindings();
    member_apply();
    member_for_each();
    free_apply();
    multiple_return_values();
    comparison();
    type_traits();
    data_pipeline();
    heterogeneous_processing();

    std::cout << "All examples completed successfully!\n";
    return 0;
}
