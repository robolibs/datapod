#include <cstring>
#include <datapod/sequential/array.hpp>
#include <iostream>

using namespace datapod;

struct Point {
    int x, y;
    friend std::ostream &operator<<(std::ostream &os, Point const &p) { return os << "(" << p.x << "," << p.y << ")"; }
};

int main() {
    std::cout << "=== Array Usage Examples ===" << std::endl << std::endl;

    // 1. Construction
    std::cout << "1. Construction:" << std::endl;
    Array<int, 5> arr1{};              // Default initialization
    Array<int, 5> arr2{1, 2, 3, 4, 5}; // Aggregate initialization
    auto arr3 = Array{10, 20, 30};     // Deduction guide

    std::cout << "   arr2[0] = " << arr2[0] << std::endl;
    std::cout << "   arr2 size = " << arr2.size() << std::endl;
    std::cout << "   arr3 deduced size = " << arr3.size() << std::endl << std::endl;

    // 2. Element Access
    std::cout << "2. Element Access:" << std::endl;
    Array<int, 5> arr{10, 20, 30, 40, 50};

    std::cout << "   arr[0] = " << arr[0] << std::endl;
    std::cout << "   arr.at(2) = " << arr.at(2) << std::endl;
    std::cout << "   arr.front() = " << arr.front() << std::endl;
    std::cout << "   arr.back() = " << arr.back() << std::endl;

    // Modify via references
    arr.front() = 100;
    arr.back() = 500;
    std::cout << "   After modification: front = " << arr.front() << ", back = " << arr.back() << std::endl
              << std::endl;

    // 3. Iterators
    std::cout << "3. Iteration:" << std::endl;
    Array<int, 5> nums{1, 2, 3, 4, 5};

    std::cout << "   Range-based for: ";
    for (auto n : nums) {
        std::cout << n << " ";
    }
    std::cout << std::endl;

    std::cout << "   Iterator: ";
    for (auto it = nums.begin(); it != nums.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl << std::endl;

    // 4. Capacity
    std::cout << "4. Capacity:" << std::endl;
    Array<int, 10> capacity_test;
    std::cout << "   size() = " << capacity_test.size() << std::endl;
    std::cout << "   max_size() = " << capacity_test.max_size() << std::endl;
    std::cout << "   empty() = " << (capacity_test.empty() ? "true" : "false") << std::endl;

    Array<int, 0> empty_arr;
    std::cout << "   Zero-size array empty() = " << (empty_arr.empty() ? "true" : "false") << std::endl << std::endl;

    // 5. Operations - fill()
    std::cout << "5. fill() Operation:" << std::endl;
    Array<int, 5> fill_test;
    fill_test.fill(42);
    std::cout << "   After fill(42): ";
    for (auto n : fill_test) {
        std::cout << n << " ";
    }
    std::cout << std::endl << std::endl;

    // 6. Operations - swap()
    std::cout << "6. swap() Operation:" << std::endl;
    Array<int, 3> swap1{1, 2, 3};
    Array<int, 3> swap2{10, 20, 30};

    std::cout << "   Before swap:" << std::endl;
    std::cout << "     swap1: ";
    for (auto n : swap1)
        std::cout << n << " ";
    std::cout << std::endl << "     swap2: ";
    for (auto n : swap2)
        std::cout << n << " ";
    std::cout << std::endl;

    swap1.swap(swap2);

    std::cout << "   After swap:" << std::endl;
    std::cout << "     swap1: ";
    for (auto n : swap1)
        std::cout << n << " ";
    std::cout << std::endl << "     swap2: ";
    for (auto n : swap2)
        std::cout << n << " ";
    std::cout << std::endl << std::endl;

    // 7. Comparison Operators
    std::cout << "7. Comparison:" << std::endl;
    Array<int, 3> cmp1{1, 2, 3};
    Array<int, 3> cmp2{1, 2, 3};
    Array<int, 3> cmp3{1, 2, 4};

    std::cout << "   cmp1 == cmp2: " << (cmp1 == cmp2 ? "true" : "false") << std::endl;
    std::cout << "   cmp1 != cmp3: " << (cmp1 != cmp3 ? "true" : "false") << std::endl;
    std::cout << "   cmp1 < cmp3:  " << (cmp1 < cmp3 ? "true" : "false") << std::endl << std::endl;

    // 8. Data pointer access
    std::cout << "8. Raw Data Access:" << std::endl;
    Array<int, 5> raw{1, 2, 3, 4, 5};
    int *ptr = raw.data();
    std::cout << "   Via data() pointer: ";
    for (size_t i = 0; i < raw.size(); ++i) {
        std::cout << ptr[i] << " ";
    }
    std::cout << std::endl << std::endl;

    // 9. Complex types
    std::cout << "9. Complex Types:" << std::endl;

    Array<Point, 3> points{Point{1, 2}, Point{3, 4}, Point{5, 6}};

    std::cout << "   Points: ";
    for (auto const &p : points) {
        std::cout << p << " ";
    }
    std::cout << std::endl << std::endl;

    // 10. Serialization Support
    std::cout << "10. Serialization (members()):" << std::endl;
    Array<int, 3> original{100, 200, 300};

    // Extract via members()
    auto [data] = original.members();
    std::cout << "   Extracted via members(): " << data[0] << ", " << data[1] << ", " << data[2] << std::endl;

    // Serialization round-trip (POD-compatible)
    Array<int, 3> copy;
    std::memcpy(&copy, &original, sizeof(original));
    std::cout << "   After memcpy: " << copy[0] << ", " << copy[1] << ", " << copy[2] << std::endl;
    std::cout << "   Match: " << (copy == original ? "YES" : "NO") << std::endl << std::endl;

    // 11. Constexpr support
    std::cout << "11. Compile-Time (constexpr):" << std::endl;
    constexpr Array<int, 3> compile_time{10, 20, 30};
    constexpr auto first = compile_time[0];
    constexpr auto sz = compile_time.size();
    std::cout << "   Constexpr array[0] = " << first << std::endl;
    std::cout << "   Constexpr size = " << sz << std::endl << std::endl;

    // 12. Bounds checking with at()
    std::cout << "12. Bounds Checking:" << std::endl;
    Array<int, 3> bounds{1, 2, 3};
    try {
        std::cout << "   bounds.at(1) = " << bounds.at(1) << std::endl;
        std::cout << "   bounds.at(10) = ";
        auto val = bounds.at(10); // This will throw
        std::cout << val << std::endl;
    } catch (std::out_of_range const &e) {
        std::cout << "Exception caught: " << e.what() << std::endl;
    }
    std::cout << std::endl;

    std::cout << "=== All Array Examples Complete ===" << std::endl;

    return 0;
}
