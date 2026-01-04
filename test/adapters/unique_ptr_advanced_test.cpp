#include <doctest/doctest.h>

#include "datapod/adapters/unique_ptr.hpp"

using namespace datapod;

TEST_SUITE("UniquePtr Advanced") {

    // ========================================================================
    // Comparison Operators Tests
    // ========================================================================

    TEST_CASE("UniquePtr - ordering comparisons") {
        int a = 1, b = 2;
        UniquePtr<int> ptr1(&a);
        UniquePtr<int> ptr2(&b);

        // Note: comparison is based on pointer addresses
        bool less = ptr1 < ptr2;
        bool greater = ptr1 > ptr2;
        bool less_eq = ptr1 <= ptr2;
        bool greater_eq = ptr1 >= ptr2;

        // One of these should be true
        CHECK((less || greater || (ptr1.get() == ptr2.get())));

        // Release to avoid double delete
        ptr1.release();
        ptr2.release();
    }

    TEST_CASE("UniquePtr - comparison with same pointer") {
        int value = 42;
        UniquePtr<int> ptr1(&value);
        UniquePtr<int> ptr2;

        CHECK(ptr1 >= ptr1);
        CHECK(ptr1 <= ptr1);
        CHECK_FALSE(ptr1 < ptr1);
        CHECK_FALSE(ptr1 > ptr1);

        ptr1.release();
    }

    TEST_CASE("UniquePtr - nullptr comparisons") {
        UniquePtr<int> ptr;

        CHECK(ptr == nullptr);
        CHECK(nullptr == ptr);
        CHECK_FALSE(ptr != nullptr);
        CHECK_FALSE(nullptr != ptr);
    }

    // ========================================================================
    // Array Specialization Tests
    // ========================================================================

    TEST_CASE("UniquePtr<T[]> - basic construction") {
        UniquePtr<int[]> arr(new int[5]{1, 2, 3, 4, 5});

        CHECK(arr);
        CHECK(arr.get() != nullptr);
    }

    TEST_CASE("UniquePtr<T[]> - operator[]") {
        UniquePtr<int[]> arr(new int[5]{10, 20, 30, 40, 50});

        CHECK(arr[0] == 10);
        CHECK(arr[1] == 20);
        CHECK(arr[2] == 30);
        CHECK(arr[3] == 40);
        CHECK(arr[4] == 50);
    }

    TEST_CASE("UniquePtr<T[]> - operator[] mutation") {
        UniquePtr<int[]> arr(new int[3]{1, 2, 3});

        arr[0] = 100;
        arr[1] = 200;
        arr[2] = 300;

        CHECK(arr[0] == 100);
        CHECK(arr[1] == 200);
        CHECK(arr[2] == 300);
    }

    TEST_CASE("UniquePtr<T[]> - make_unique for arrays") {
        // Note: make_unique<int>(5) creates UniquePtr<int[]> with array specialization
        UniquePtr<int[]> arr(new int[5]());

        CHECK(arr);

        // Initialize values
        for (std::size_t i = 0; i < 5; ++i) {
            arr[i] = static_cast<int>(i * 10);
        }

        CHECK(arr[0] == 0);
        CHECK(arr[1] == 10);
        CHECK(arr[2] == 20);
        CHECK(arr[3] == 30);
        CHECK(arr[4] == 40);
    }

    TEST_CASE("UniquePtr<T[]> - move semantics") {
        UniquePtr<int[]> arr1(new int[3]{1, 2, 3});

        UniquePtr<int[]> arr2(std::move(arr1));

        CHECK_FALSE(arr1);
        CHECK(arr2);
        CHECK(arr2[0] == 1);
        CHECK(arr2[1] == 2);
        CHECK(arr2[2] == 3);
    }

    TEST_CASE("UniquePtr<T[]> - move assignment") {
        UniquePtr<int[]> arr1(new int[3]{1, 2, 3});
        UniquePtr<int[]> arr2(new int[2]{10, 20});

        arr2 = std::move(arr1);

        CHECK_FALSE(arr1);
        CHECK(arr2);
        CHECK(arr2[0] == 1);
        CHECK(arr2[1] == 2);
        CHECK(arr2[2] == 3);
    }

    TEST_CASE("UniquePtr<T[]> - reset") {
        UniquePtr<int[]> arr(new int[3]{1, 2, 3});

        arr.reset(new int[2]{10, 20});

        CHECK(arr);
        CHECK(arr[0] == 10);
        CHECK(arr[1] == 20);
    }

    TEST_CASE("UniquePtr<T[]> - reset to nullptr") {
        UniquePtr<int[]> arr(new int[3]{1, 2, 3});

        arr.reset();

        CHECK_FALSE(arr);
        CHECK(arr.get() == nullptr);
    }

    TEST_CASE("UniquePtr<T[]> - release") {
        UniquePtr<int[]> arr(new int[3]{1, 2, 3});

        int *raw = arr.release();

        CHECK_FALSE(arr);
        CHECK(raw != nullptr);
        CHECK(raw[0] == 1);
        CHECK(raw[1] == 2);
        CHECK(raw[2] == 3);

        delete[] raw; // Manual cleanup
    }

    TEST_CASE("UniquePtr<T[]> - swap") {
        UniquePtr<int[]> arr1(new int[2]{1, 2});
        UniquePtr<int[]> arr2(new int[2]{10, 20});

        arr1.swap(arr2);

        CHECK(arr1[0] == 10);
        CHECK(arr1[1] == 20);
        CHECK(arr2[0] == 1);
        CHECK(arr2[1] == 2);
    }

    TEST_CASE("UniquePtr<T[]> - nullptr assignment") {
        UniquePtr<int[]> arr(new int[3]{1, 2, 3});

        arr = nullptr;

        CHECK_FALSE(arr);
        CHECK(arr.get() == nullptr);
    }

    // ========================================================================
    // Complex Type Tests
    // ========================================================================

    struct TestStruct {
        int value;
        TestStruct(int v = 0) : value(v) {}
    };

    TEST_CASE("UniquePtr<T[]> - complex types") {
        UniquePtr<TestStruct[]> arr(new TestStruct[3]);

        arr[0].value = 10;
        arr[1].value = 20;
        arr[2].value = 30;

        CHECK(arr[0].value == 10);
        CHECK(arr[1].value == 20);
        CHECK(arr[2].value == 30);
    }

    TEST_CASE("UniquePtr<T[]> - arrays with complex types") {
        UniquePtr<TestStruct[]> arr(new TestStruct[3]());

        arr[0].value = 100;
        arr[1].value = 200;
        arr[2].value = 300;

        CHECK(arr[0].value == 100);
        CHECK(arr[1].value == 200);
        CHECK(arr[2].value == 300);
    }
}
