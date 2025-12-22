#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "datapod/sequential/array.hpp"

using namespace datapod;

TEST_SUITE("Array") {

    // ========================================================================
    // Construction
    // ========================================================================

    TEST_CASE("DefaultConstruction") {
        Array<int, 5> arr{};
        CHECK(arr.size() == 5);
        CHECK_FALSE(arr.empty());
    }

    TEST_CASE("AggregateInitialization") {
        Array<int, 5> arr{1, 2, 3, 4, 5};
        CHECK(arr[0] == 1);
        CHECK(arr[1] == 2);
        CHECK(arr[2] == 3);
        CHECK(arr[3] == 4);
        CHECK(arr[4] == 5);
    }

    TEST_CASE("DeductionGuide") {
        auto arr = Array{1, 2, 3, 4, 5};
        CHECK(arr.size() == 5);
        CHECK(arr[0] == 1);
        CHECK(arr[4] == 5);
    }

    TEST_CASE("ZeroSizeArray") {
        Array<int, 0> arr;
        CHECK(arr.size() == 0);
        CHECK(arr.empty());
        CHECK(arr.data() == nullptr);
    }

    // ========================================================================
    // Element Access
    // ========================================================================

    TEST_CASE("OperatorBracket") {
        Array<int, 5> arr{1, 2, 3, 4, 5};
        CHECK(arr[0] == 1);
        CHECK(arr[4] == 5);

        arr[2] = 99;
        CHECK(arr[2] == 99);
    }

    TEST_CASE("OperatorBracketConst") {
        Array<int, 3> const arr{10, 20, 30};
        CHECK(arr[0] == 10);
        CHECK(arr[1] == 20);
        CHECK(arr[2] == 30);
    }

    TEST_CASE("At") {
        Array<int, 5> arr{1, 2, 3, 4, 5};
        CHECK(arr.at(0) == 1);
        CHECK(arr.at(4) == 5);

        arr.at(2) = 99;
        CHECK(arr.at(2) == 99);
    }

    TEST_CASE("AtOutOfBounds") {
        Array<int, 5> arr{1, 2, 3, 4, 5};
        CHECK_THROWS_AS(arr.at(5), std::out_of_range);
        CHECK_THROWS_AS(arr.at(100), std::out_of_range);
    }

    TEST_CASE("AtConst") {
        Array<int, 3> const arr{10, 20, 30};
        CHECK(arr.at(0) == 10);
        CHECK(arr.at(2) == 30);
    }

    TEST_CASE("AtConstOutOfBounds") {
        Array<int, 3> const arr{10, 20, 30};
        CHECK_THROWS_AS(arr.at(3), std::out_of_range);
    }

    TEST_CASE("Front") {
        Array<int, 5> arr{1, 2, 3, 4, 5};
        CHECK(arr.front() == 1);

        arr.front() = 99;
        CHECK(arr.front() == 99);
        CHECK(arr[0] == 99);
    }

    TEST_CASE("FrontConst") {
        Array<int, 3> const arr{10, 20, 30};
        CHECK(arr.front() == 10);
    }

    TEST_CASE("Back") {
        Array<int, 5> arr{1, 2, 3, 4, 5};
        CHECK(arr.back() == 5);

        arr.back() = 99;
        CHECK(arr.back() == 99);
        CHECK(arr[4] == 99);
    }

    TEST_CASE("BackConst") {
        Array<int, 3> const arr{10, 20, 30};
        CHECK(arr.back() == 30);
    }

    TEST_CASE("Data") {
        Array<int, 5> arr{1, 2, 3, 4, 5};
        int *ptr = arr.data();
        CHECK(ptr != nullptr);
        CHECK(ptr[0] == 1);
        CHECK(ptr[4] == 5);

        ptr[2] = 99;
        CHECK(arr[2] == 99);
    }

    TEST_CASE("DataConst") {
        Array<int, 3> const arr{10, 20, 30};
        int const *ptr = arr.data();
        CHECK(ptr != nullptr);
        CHECK(ptr[0] == 10);
        CHECK(ptr[2] == 30);
    }

    // ========================================================================
    // Iterators
    // ========================================================================

    TEST_CASE("Iterators") {
        Array<int, 5> arr{1, 2, 3, 4, 5};

        auto it = arr.begin();
        CHECK(*it == 1);
        ++it;
        CHECK(*it == 2);

        auto end = arr.end();
        CHECK(end - arr.begin() == 5);
    }

    TEST_CASE("ConstIterators") {
        Array<int, 5> const arr{1, 2, 3, 4, 5};

        auto it = arr.begin();
        CHECK(*it == 1);

        auto cit = arr.cbegin();
        CHECK(*cit == 1);
    }

    TEST_CASE("RangeBasedFor") {
        Array<int, 5> arr{1, 2, 3, 4, 5};

        int sum = 0;
        for (auto val : arr) {
            sum += val;
        }
        CHECK(sum == 15);
    }

    TEST_CASE("RangeBasedForModify") {
        Array<int, 5> arr{1, 2, 3, 4, 5};

        for (auto &val : arr) {
            val *= 2;
        }

        CHECK(arr[0] == 2);
        CHECK(arr[1] == 4);
        CHECK(arr[2] == 6);
        CHECK(arr[3] == 8);
        CHECK(arr[4] == 10);
    }

    // ========================================================================
    // Capacity
    // ========================================================================

    TEST_CASE("Size") {
        Array<int, 5> arr;
        CHECK(arr.size() == 5);

        Array<int, 100> large;
        CHECK(large.size() == 100);

        Array<int, 1> tiny;
        CHECK(tiny.size() == 1);
    }

    TEST_CASE("Empty") {
        Array<int, 5> arr;
        CHECK_FALSE(arr.empty());

        Array<int, 0> empty;
        CHECK(empty.empty());
    }

    TEST_CASE("MaxSize") {
        Array<int, 5> arr;
        CHECK(arr.max_size() == 5);
        CHECK(arr.max_size() == arr.size());
    }

    // ========================================================================
    // Operations
    // ========================================================================

    TEST_CASE("Fill") {
        Array<int, 5> arr{1, 2, 3, 4, 5};
        arr.fill(42);

        CHECK(arr[0] == 42);
        CHECK(arr[1] == 42);
        CHECK(arr[2] == 42);
        CHECK(arr[3] == 42);
        CHECK(arr[4] == 42);
    }

    TEST_CASE("FillString") {
        Array<std::string, 3> arr;
        arr.fill("hello");

        CHECK(arr[0] == "hello");
        CHECK(arr[1] == "hello");
        CHECK(arr[2] == "hello");
    }

    TEST_CASE("Swap") {
        Array<int, 5> arr1{1, 2, 3, 4, 5};
        Array<int, 5> arr2{10, 20, 30, 40, 50};

        arr1.swap(arr2);

        CHECK(arr1[0] == 10);
        CHECK(arr1[4] == 50);
        CHECK(arr2[0] == 1);
        CHECK(arr2[4] == 5);
    }

    TEST_CASE("SwapEmpty") {
        Array<int, 0> arr1;
        Array<int, 0> arr2;
        arr1.swap(arr2); // Should not crash
        CHECK(arr1.empty());
        CHECK(arr2.empty());
    }

    // ========================================================================
    // Comparison Operators
    // ========================================================================

    TEST_CASE("Equality") {
        Array<int, 5> arr1{1, 2, 3, 4, 5};
        Array<int, 5> arr2{1, 2, 3, 4, 5};
        Array<int, 5> arr3{1, 2, 3, 4, 6};

        CHECK(arr1 == arr2);
        CHECK_FALSE(arr1 == arr3);
        CHECK(arr1 != arr3);
        CHECK_FALSE(arr1 != arr2);
    }

    TEST_CASE("LessThan") {
        Array<int, 5> arr1{1, 2, 3, 4, 5};
        Array<int, 5> arr2{1, 2, 3, 4, 6};
        Array<int, 5> arr3{1, 2, 3, 4, 5};

        CHECK(arr1 < arr2);
        CHECK_FALSE(arr2 < arr1);
        CHECK_FALSE(arr1 < arr3);
    }

    TEST_CASE("LessThanOrEqual") {
        Array<int, 5> arr1{1, 2, 3, 4, 5};
        Array<int, 5> arr2{1, 2, 3, 4, 6};
        Array<int, 5> arr3{1, 2, 3, 4, 5};

        CHECK(arr1 <= arr2);
        CHECK(arr1 <= arr3);
        CHECK_FALSE(arr2 <= arr1);
    }

    TEST_CASE("GreaterThan") {
        Array<int, 5> arr1{1, 2, 3, 4, 6};
        Array<int, 5> arr2{1, 2, 3, 4, 5};

        CHECK(arr1 > arr2);
        CHECK_FALSE(arr2 > arr1);
    }

    TEST_CASE("GreaterThanOrEqual") {
        Array<int, 5> arr1{1, 2, 3, 4, 6};
        Array<int, 5> arr2{1, 2, 3, 4, 5};
        Array<int, 5> arr3{1, 2, 3, 4, 5};

        CHECK(arr1 >= arr2);
        CHECK(arr2 >= arr3);
        CHECK_FALSE(arr2 >= arr1);
    }

    // ========================================================================
    // Serialization
    // ========================================================================

    TEST_CASE("Members") {
        Array<int, 5> arr{1, 2, 3, 4, 5};
        auto members = arr.members();

        // members() should return a tuple with the data array
        auto &[data] = members;
        CHECK(data[0] == 1);
        CHECK(data[4] == 5);
    }

    TEST_CASE("MembersSerialization") {
        Array<int, 3> original{10, 20, 30};

        // Verify members() works for serialization
        auto [data] = original.members();

        Array<int, 3> copy;
        auto [copy_data] = copy.members();
        std::memcpy(&copy_data, &data, sizeof(data));

        CHECK(copy[0] == 10);
        CHECK(copy[1] == 20);
        CHECK(copy[2] == 30);
    }

    // ========================================================================
    // Edge Cases
    // ========================================================================

    TEST_CASE("LargeArray") {
        Array<int, 1000> arr;
        arr.fill(42);

        CHECK(arr.size() == 1000);
        CHECK(arr[0] == 42);
        CHECK(arr[999] == 42);
    }

    TEST_CASE("SingleElement") {
        Array<int, 1> arr{42};
        CHECK(arr.size() == 1);
        CHECK(arr.front() == 42);
        CHECK(arr.back() == 42);
        CHECK(arr[0] == 42);
    }

    TEST_CASE("ComplexType") {
        struct Complex {
            int x;
            std::string s;
            bool operator==(Complex const &o) const { return x == o.x && s == o.s; }
        };

        Array<Complex, 3> arr{Complex{1, "one"}, Complex{2, "two"}, Complex{3, "three"}};

        CHECK(arr[0].x == 1);
        CHECK(arr[1].s == "two");
        CHECK(arr[2].x == 3);
    }

    TEST_CASE("ConstexprOperations") {
        constexpr Array<int, 5> arr{1, 2, 3, 4, 5};
        static_assert(arr.size() == 5, "Size should be 5");
        static_assert(arr[0] == 1, "First element should be 1");
        static_assert(arr[4] == 5, "Last element should be 5");
        static_assert(!arr.empty(), "Array should not be empty");
    }
}
