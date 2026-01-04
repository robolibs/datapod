#include "datapod/pods/adapters/pair.hpp"
#include <iostream>
#include <string>

using namespace datapod;

// Example 1: Basic Pair Usage
void basic_usage() {
    std::cout << "=== Example 1: Basic Pair Usage ===\n";

    // Create a pair
    Pair<int, std::string> p(42, "hello");
    std::cout << "Pair: (" << p.first << ", " << p.second << ")\n";

    // Using make_pair
    auto p2 = datapod::make_pair(100, 3.14);
    std::cout << "Auto pair: (" << p2.first << ", " << p2.second << ")\n";

    // Deduction guide
    Pair p3(true, 'A');
    std::cout << "Deduced pair: (" << p3.first << ", " << p3.second << ")\n";
    std::cout << "\n";
}

// Example 2: Structured Bindings
void structured_bindings() {
    std::cout << "=== Example 2: Structured Bindings ===\n";

    Pair<int, std::string> p(42, "world");

    // Decompose into separate variables
    auto [num, str] = p;
    std::cout << "Decomposed: num=" << num << ", str=" << str << "\n";

    // Modify via structured binding references
    auto &[n, s] = p;
    n = 100;
    s = "modified";
    std::cout << "After modification: (" << p.first << ", " << p.second << ")\n";

    // Const structured bindings
    Pair<double, int> const cp(3.14, 42);
    auto const &[pi, answer] = cp;
    std::cout << "Const pair: pi=" << pi << ", answer=" << answer << "\n";
    std::cout << "\n";
}

// Example 3: Comparison and Swap
void comparison_and_swap() {
    std::cout << "=== Example 3: Comparison and Swap ===\n";

    Pair<int, int> p1(1, 2);
    Pair<int, int> p2(1, 3);
    Pair<int, int> p3(2, 1);

    std::cout << "p1 == p2: " << (p1 == p2) << "\n";
    std::cout << "p1 < p2: " << (p1 < p2) << "\n";
    std::cout << "p1 < p3: " << (p1 < p3) << "\n";

    // Swap
    Pair<int, std::string> a(42, "foo");
    Pair<int, std::string> b(100, "bar");
    std::cout << "Before swap: a=(" << a.first << "," << a.second << "), b=(" << b.first << "," << b.second << ")\n";

    datapod::swap(a, b);
    std::cout << "After swap:  a=(" << a.first << "," << a.second << "), b=(" << b.first << "," << b.second << ")\n";
    std::cout << "\n";
}

// Example 4: get<I>() Access
void get_access() {
    std::cout << "=== Example 4: get<I>() Access ===\n";

    Pair<double, std::string> p(3.14159, "pi");

    // Access via member get
    std::cout << "First (member): " << p.get<0>() << "\n";
    std::cout << "Second (member): " << p.get<1>() << "\n";

    // Access via free function get
    std::cout << "First (free): " << datapod::get<0>(p) << "\n";
    std::cout << "Second (free): " << datapod::get<1>(p) << "\n";

    // Modify via get
    datapod::get<0>(p) = 2.71828;
    datapod::get<1>(p) = "e";
    std::cout << "Modified: (" << p.first << ", " << p.second << ")\n";
    std::cout << "\n";
}

// Example 5: Return Multiple Values
Pair<int, int> divide_with_remainder(int a, int b) { return Pair(a / b, a % b); }

void multiple_return_values() {
    std::cout << "=== Example 5: Multiple Return Values ===\n";

    auto [quotient, remainder] = divide_with_remainder(17, 5);
    std::cout << "17 / 5 = " << quotient << " remainder " << remainder << "\n";

    // Can also use without structured bindings
    auto result = divide_with_remainder(23, 7);
    std::cout << "23 / 7 = " << result.first << " remainder " << result.second << "\n";
    std::cout << "\n";
}

// Example 6: Pair in Containers
void pair_in_containers() {
    std::cout << "=== Example 6: Complex Types ===\n";

    // Pair of pairs (coordinates)
    Pair<Pair<int, int>, std::string> labeled_point(Pair(10, 20), "origin");
    auto const &[coords, label] = labeled_point;
    std::cout << "Point '" << label << "' at (" << coords.first << ", " << coords.second << ")\n";

    // Pair with move-only type semantics
    Pair<int, std::string> p(42, "moveable");
    auto p2 = std::move(p);
    std::cout << "Moved pair: (" << p2.first << ", " << p2.second << ")\n";
    std::cout << "\n";
}

// Example 7: Type Traits
void type_traits() {
    std::cout << "=== Example 7: Type Traits ===\n";

    using P = Pair<int, double>;

    std::cout << "tuple_size: " << std::tuple_size_v<P> << "\n";
    std::cout << "First type is int: " << std::is_same_v<std::tuple_element_t<0, P>, int> << "\n";
    std::cout << "Second type is double: " << std::is_same_v<std::tuple_element_t<1, P>, double> << "\n";
    std::cout << "\n";
}

int main() {
    std::cout << "Datapod Pair Usage Examples\n";
    std::cout << "============================\n\n";

    basic_usage();
    structured_bindings();
    comparison_and_swap();
    get_access();
    multiple_return_values();
    pair_in_containers();
    type_traits();

    std::cout << "All examples completed successfully!\n";
    return 0;
}
