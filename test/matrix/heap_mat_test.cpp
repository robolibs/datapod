#include <doctest/doctest.h>

#include "datapod/matrix/matrix.hpp"
#include "datapod/matrix/tensor.hpp"
#include "datapod/matrix/vector.hpp"
#include "datapod/serialization/serialize.hpp"

using namespace datapod;
using namespace datapod::mat;

// =============================================================================
// HEAP-ALLOCATED VECTOR TESTS
// =============================================================================

TEST_SUITE("heap mat::vector") {
    // Use a size larger than HEAP_THRESHOLD (1024)
    static constexpr size_t LARGE_SIZE = 2000;

    TEST_CASE("heap vector type traits") {
        // Small vectors use stack
        CHECK_FALSE(vector<double, 3>::uses_heap);
        CHECK(vector<double, 3>::is_pod);

        // Large vectors use heap
        CHECK(vector<double, LARGE_SIZE>::uses_heap);
        CHECK_FALSE(vector<double, LARGE_SIZE>::is_pod);

        // Type trait detection
        CHECK(is_heap_vector_v<vector<double, LARGE_SIZE>>);
        CHECK_FALSE(is_heap_vector_v<vector<double, 3>>);
    }

    TEST_CASE("heap vector construction and access") {
        vector<double, LARGE_SIZE> v;

        // Should be zero-initialized
        for (size_t i = 0; i < LARGE_SIZE; ++i) {
            CHECK(v[i] == 0.0);
        }

        // Set values
        v[0] = 1.0;
        v[100] = 100.0;
        v[LARGE_SIZE - 1] = 999.0;

        CHECK(v[0] == 1.0);
        CHECK(v[100] == 100.0);
        CHECK(v[LARGE_SIZE - 1] == 999.0);
    }

    TEST_CASE("heap vector copy") {
        vector<double, LARGE_SIZE> v1;
        v1[0] = 42.0;
        v1[500] = 500.0;

        // Copy constructor
        vector<double, LARGE_SIZE> v2(v1);
        CHECK(v2[0] == 42.0);
        CHECK(v2[500] == 500.0);

        // Copy assignment
        vector<double, LARGE_SIZE> v3;
        v3 = v1;
        CHECK(v3[0] == 42.0);
        CHECK(v3[500] == 500.0);

        // Verify independence
        v1[0] = 999.0;
        CHECK(v2[0] == 42.0);
        CHECK(v3[0] == 42.0);
    }

    TEST_CASE("heap vector move") {
        vector<double, LARGE_SIZE> v1;
        v1[0] = 42.0;
        v1[500] = 500.0;

        // Move constructor
        vector<double, LARGE_SIZE> v2(std::move(v1));
        CHECK(v2[0] == 42.0);
        CHECK(v2[500] == 500.0);

        // Move assignment
        vector<double, LARGE_SIZE> v3;
        v3[0] = 1.0;
        vector<double, LARGE_SIZE> v4;
        v4[0] = 999.0;
        v3 = std::move(v4);
        CHECK(v3[0] == 999.0);
    }

    TEST_CASE("heap vector fill and swap") {
        vector<double, LARGE_SIZE> v1;
        v1.fill(7.0);
        for (size_t i = 0; i < LARGE_SIZE; ++i) {
            CHECK(v1[i] == 7.0);
        }

        vector<double, LARGE_SIZE> v2;
        v2.fill(3.0);

        v1.swap(v2);
        CHECK(v1[0] == 3.0);
        CHECK(v2[0] == 7.0);
    }

    TEST_CASE("heap vector iterators") {
        vector<int, LARGE_SIZE> v;
        for (size_t i = 0; i < LARGE_SIZE; ++i) {
            v[i] = static_cast<int>(i);
        }

        // Range-based for
        int sum = 0;
        int count = 0;
        for (auto val : v) {
            sum += val;
            ++count;
        }
        CHECK(count == LARGE_SIZE);
        // Sum of 0..1999 = 1999*2000/2 = 1999000
        CHECK(sum == 1999000);
    }

    TEST_CASE("heap vector comparison") {
        vector<double, LARGE_SIZE> v1;
        vector<double, LARGE_SIZE> v2;
        v1.fill(5.0);
        v2.fill(5.0);

        CHECK(v1 == v2);

        v2[100] = 6.0;
        CHECK(v1 != v2);
    }

    TEST_CASE("heap vector SIMD alignment") {
        vector<double, LARGE_SIZE> v;
        // Should be 32-byte aligned for SIMD
        CHECK(reinterpret_cast<uintptr_t>(v.data()) % 32 == 0);
    }

    TEST_CASE("heap vector serialization") {
        vector<float, LARGE_SIZE> original;
        for (size_t i = 0; i < LARGE_SIZE; ++i) {
            original[i] = static_cast<float>(i) * 0.5f;
        }

        // Serialize
        auto buf = serialize(original);

        // Deserialize
        auto restored = deserialize<Mode::NONE, vector<float, LARGE_SIZE>>(buf);

        // Verify
        for (size_t i = 0; i < LARGE_SIZE; ++i) {
            CHECK(restored[i] == original[i]);
        }
    }
}

// =============================================================================
// HEAP-ALLOCATED MATRIX TESTS
// =============================================================================

TEST_SUITE("heap mat::matrix") {
    // Use dimensions where R*C > HEAP_THRESHOLD (1024)
    static constexpr size_t ROWS = 50;
    static constexpr size_t COLS = 50; // 2500 elements

    TEST_CASE("heap matrix type traits") {
        // Small matrices use stack
        CHECK_FALSE(matrix<double, 3, 3>::uses_heap);
        CHECK(matrix<double, 3, 3>::is_pod);

        // Large matrices use heap
        CHECK(matrix<double, ROWS, COLS>::uses_heap);
        CHECK_FALSE(matrix<double, ROWS, COLS>::is_pod);

        // Type trait detection
        CHECK(is_heap_matrix_v<matrix<double, ROWS, COLS>>);
        CHECK_FALSE(is_heap_matrix_v<matrix<double, 3, 3>>);
    }

    TEST_CASE("heap matrix construction and access") {
        matrix<double, ROWS, COLS> m;

        // Should be zero-initialized
        for (size_t r = 0; r < ROWS; ++r) {
            for (size_t c = 0; c < COLS; ++c) {
                CHECK(m(r, c) == 0.0);
            }
        }

        // Set values
        m(0, 0) = 1.0;
        m(10, 20) = 42.0;
        m(ROWS - 1, COLS - 1) = 999.0;

        CHECK(m(0, 0) == 1.0);
        CHECK(m(10, 20) == 42.0);
        CHECK(m(ROWS - 1, COLS - 1) == 999.0);
    }

    TEST_CASE("heap matrix copy") {
        matrix<double, ROWS, COLS> m1;
        m1(5, 5) = 55.0;
        m1(10, 10) = 100.0;

        // Copy constructor
        matrix<double, ROWS, COLS> m2(m1);
        CHECK(m2(5, 5) == 55.0);
        CHECK(m2(10, 10) == 100.0);

        // Copy assignment
        matrix<double, ROWS, COLS> m3;
        m3 = m1;
        CHECK(m3(5, 5) == 55.0);

        // Verify independence
        m1(5, 5) = 999.0;
        CHECK(m2(5, 5) == 55.0);
        CHECK(m3(5, 5) == 55.0);
    }

    TEST_CASE("heap matrix move") {
        matrix<double, ROWS, COLS> m1;
        m1(5, 5) = 55.0;

        // Move constructor
        matrix<double, ROWS, COLS> m2(std::move(m1));
        CHECK(m2(5, 5) == 55.0);

        // Move assignment
        matrix<double, ROWS, COLS> m3;
        matrix<double, ROWS, COLS> m4;
        m4(0, 0) = 999.0;
        m3 = std::move(m4);
        CHECK(m3(0, 0) == 999.0);
    }

    TEST_CASE("heap matrix fill and identity") {
        matrix<double, ROWS, COLS> m;
        m.fill(3.14);
        for (size_t i = 0; i < ROWS * COLS; ++i) {
            CHECK(m[i] == 3.14);
        }
    }

    TEST_CASE("heap square matrix identity") {
        matrix<double, 40, 40> m; // Square, heap-allocated
        m.set_identity();
        for (size_t r = 0; r < 40; ++r) {
            for (size_t c = 0; c < 40; ++c) {
                if (r == c) {
                    CHECK(m(r, c) == 1.0);
                } else {
                    CHECK(m(r, c) == 0.0);
                }
            }
        }
    }

    TEST_CASE("heap matrix iterators") {
        matrix<int, ROWS, COLS> m;
        int val = 0;
        for (size_t i = 0; i < ROWS * COLS; ++i) {
            m[i] = val++;
        }

        // Range-based for
        int sum = 0;
        for (auto v : m) {
            sum += v;
        }
        // Sum of 0..2499 = 2499*2500/2 = 3123750
        CHECK(sum == 3123750);
    }

    TEST_CASE("heap matrix comparison") {
        matrix<double, ROWS, COLS> m1;
        matrix<double, ROWS, COLS> m2;
        m1.fill(2.0);
        m2.fill(2.0);

        CHECK(m1 == m2);

        m2(0, 0) = 3.0;
        CHECK(m1 != m2);
    }

    TEST_CASE("heap matrix SIMD alignment") {
        matrix<double, ROWS, COLS> m;
        CHECK(reinterpret_cast<uintptr_t>(m.data()) % 32 == 0);
    }

    TEST_CASE("heap matrix serialization") {
        matrix<float, ROWS, COLS> original;
        for (size_t i = 0; i < ROWS * COLS; ++i) {
            original[i] = static_cast<float>(i) * 0.1f;
        }

        auto buf = serialize(original);
        auto restored = deserialize<Mode::NONE, matrix<float, ROWS, COLS>>(buf);

        for (size_t i = 0; i < ROWS * COLS; ++i) {
            CHECK(restored[i] == original[i]);
        }
    }
}

// =============================================================================
// HEAP-ALLOCATED TENSOR TESTS
// =============================================================================

TEST_SUITE("heap mat::heap_tensor") {
    // Use heap_tensor directly for large tensors
    static constexpr size_t D1 = 20;
    static constexpr size_t D2 = 20;
    static constexpr size_t D3 = 20; // 8000 elements

    TEST_CASE("heap_tensor type traits") {
        // Regular tensor (small) uses stack
        CHECK_FALSE(tensor<double, 3, 3, 3>::uses_heap);
        CHECK(tensor<double, 3, 3, 3>::is_pod);

        // heap_tensor uses heap
        CHECK(heap_tensor<double, D1, D2, D3>::uses_heap);
        CHECK_FALSE(heap_tensor<double, D1, D2, D3>::is_pod);

        // Type trait detection
        CHECK(is_heap_tensor_v<heap_tensor<double, D1, D2, D3>>);
        CHECK_FALSE(is_heap_tensor_v<tensor<double, 3, 3, 3>>);
    }

    TEST_CASE("heap_tensor construction and access") {
        heap_tensor<double, D1, D2, D3> t;

        // Should be zero-initialized
        for (size_t i = 0; i < D1; ++i) {
            for (size_t j = 0; j < D2; ++j) {
                for (size_t k = 0; k < D3; ++k) {
                    CHECK(t(i, j, k) == 0.0);
                }
            }
        }

        // Set values
        t(0, 0, 0) = 1.0;
        t(5, 10, 15) = 42.0;
        t(D1 - 1, D2 - 1, D3 - 1) = 999.0;

        CHECK(t(0, 0, 0) == 1.0);
        CHECK(t(5, 10, 15) == 42.0);
        CHECK(t(D1 - 1, D2 - 1, D3 - 1) == 999.0);
    }

    TEST_CASE("heap_tensor copy") {
        heap_tensor<double, D1, D2, D3> t1;
        t1(1, 2, 3) = 123.0;

        // Copy constructor
        heap_tensor<double, D1, D2, D3> t2(t1);
        CHECK(t2(1, 2, 3) == 123.0);

        // Copy assignment
        heap_tensor<double, D1, D2, D3> t3;
        t3 = t1;
        CHECK(t3(1, 2, 3) == 123.0);

        // Independence
        t1(1, 2, 3) = 999.0;
        CHECK(t2(1, 2, 3) == 123.0);
        CHECK(t3(1, 2, 3) == 123.0);
    }

    TEST_CASE("heap_tensor move") {
        heap_tensor<double, D1, D2, D3> t1;
        t1(1, 2, 3) = 123.0;

        // Move constructor
        heap_tensor<double, D1, D2, D3> t2(std::move(t1));
        CHECK(t2(1, 2, 3) == 123.0);

        // Move assignment
        heap_tensor<double, D1, D2, D3> t3;
        heap_tensor<double, D1, D2, D3> t4;
        t4(0, 0, 0) = 999.0;
        t3 = std::move(t4);
        CHECK(t3(0, 0, 0) == 999.0);
    }

    TEST_CASE("heap_tensor fill and swap") {
        heap_tensor<double, D1, D2, D3> t1;
        t1.fill(7.0);
        for (size_t i = 0; i < D1 * D2 * D3; ++i) {
            CHECK(t1[i] == 7.0);
        }

        heap_tensor<double, D1, D2, D3> t2;
        t2.fill(3.0);

        t1.swap(t2);
        CHECK(t1[0] == 3.0);
        CHECK(t2[0] == 7.0);
    }

    TEST_CASE("heap_tensor shape and dimensions") {
        heap_tensor<double, D1, D2, D3> t;

        CHECK(t.size() == D1 * D2 * D3);
        CHECK(heap_tensor<double, D1, D2, D3>::rank == 3);
        CHECK(heap_tensor<double, D1, D2, D3>::dim(0) == D1);
        CHECK(heap_tensor<double, D1, D2, D3>::dim(1) == D2);
        CHECK(heap_tensor<double, D1, D2, D3>::dim(2) == D3);

        auto shape = heap_tensor<double, D1, D2, D3>::shape();
        CHECK(shape[0] == D1);
        CHECK(shape[1] == D2);
        CHECK(shape[2] == D3);
    }

    TEST_CASE("heap_tensor iterators") {
        heap_tensor<int, 10, 10, 10> t; // 1000 elements
        int val = 0;
        for (size_t i = 0; i < 1000; ++i) {
            t[i] = val++;
        }

        int sum = 0;
        for (auto v : t) {
            sum += v;
        }
        // Sum of 0..999 = 999*1000/2 = 499500
        CHECK(sum == 499500);
    }

    TEST_CASE("heap_tensor comparison") {
        heap_tensor<double, D1, D2, D3> t1;
        heap_tensor<double, D1, D2, D3> t2;
        t1.fill(5.0);
        t2.fill(5.0);

        CHECK(t1 == t2);

        t2(0, 0, 0) = 6.0;
        CHECK(t1 != t2);
    }

    TEST_CASE("heap_tensor SIMD alignment") {
        heap_tensor<double, D1, D2, D3> t;
        CHECK(reinterpret_cast<uintptr_t>(t.data()) % 32 == 0);
    }

    TEST_CASE("heap_tensor serialization") {
        heap_tensor<float, 10, 10, 10> original; // 1000 elements
        for (size_t i = 0; i < 1000; ++i) {
            original[i] = static_cast<float>(i) * 0.01f;
        }

        auto buf = serialize(original);
        auto restored = deserialize<Mode::NONE, heap_tensor<float, 10, 10, 10>>(buf);

        for (size_t i = 0; i < 1000; ++i) {
            CHECK(restored[i] == original[i]);
        }
    }
}

// =============================================================================
// BOUNDARY TESTS - AT THRESHOLD
// =============================================================================

TEST_SUITE("heap mat threshold boundary") {
    TEST_CASE("vector at threshold boundary") {
        // At threshold - should be stack
        CHECK_FALSE(vector<double, 1024>::uses_heap);
        CHECK(vector<double, 1024>::is_pod);

        // Just over threshold - should be heap
        CHECK(vector<double, 1025>::uses_heap);
        CHECK_FALSE(vector<double, 1025>::is_pod);
    }

    TEST_CASE("matrix at threshold boundary") {
        // 32x32 = 1024 elements - at threshold, should be stack
        CHECK_FALSE(matrix<double, 32, 32>::uses_heap);
        CHECK(matrix<double, 32, 32>::is_pod);

        // 33x32 = 1056 elements - over threshold, should be heap
        CHECK(matrix<double, 33, 32>::uses_heap);
        CHECK_FALSE(matrix<double, 33, 32>::is_pod);
    }

    TEST_CASE("verify HEAP_THRESHOLD value") { CHECK(HEAP_THRESHOLD == 1024); }
}
