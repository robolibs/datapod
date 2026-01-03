#include <doctest/doctest.h>

#include "datapod/matrix/scalar.hpp"
#include "datapod/matrix/tensor.hpp"
#include "datapod/reflection/for_each_field.hpp"
#include "datapod/reflection/to_tuple.hpp"

using namespace datapod;
// Not using datapod::mat to avoid Vector conflict

TEST_SUITE("mat::tensor") {
    TEST_CASE("construction 3D") {
        mat::Tensor<double, 2, 3, 4> t;
        t.fill(0.0);
        CHECK(t.size() == 24);
        CHECK(mat::Tensor<double, 2, 3, 4>::rank == 3);
    }

    TEST_CASE("multi-dimensional indexing") {
        mat::Tensor<int, 2, 3, 4> t;
        t.fill(0);

        // Set specific element
        t(0, 0, 0) = 1;
        t(1, 2, 3) = 42;
        t(0, 1, 2) = 7;

        CHECK(t(0, 0, 0) == 1);
        CHECK(t(1, 2, 3) == 42);
        CHECK(t(0, 1, 2) == 7);
    }

    TEST_CASE("checked access") {
        mat::Tensor<double, 2, 2, 2> t;
        t.fill(1.0);

        CHECK(t.at(0, 0, 0) == 1.0);
        CHECK(t.at(1, 1, 1) == 1.0);

        CHECK_THROWS_AS(t.at(2, 0, 0), std::out_of_range);
        CHECK_THROWS_AS(t.at(0, 2, 0), std::out_of_range);
        CHECK_THROWS_AS(t.at(0, 0, 2), std::out_of_range);
    }

    TEST_CASE("linear indexing") {
        mat::Tensor<int, 2, 2, 2> t;
        for (size_t i = 0; i < 8; ++i) {
            t[i] = static_cast<int>(i);
        }

        CHECK(t[0] == 0);
        CHECK(t[7] == 7);
    }

    TEST_CASE("shape and dimensions") {
        mat::Tensor<double, 3, 4, 5> t;

        auto shape = t.shape();
        CHECK(shape[0] == 3);
        CHECK(shape[1] == 4);
        CHECK(shape[2] == 5);

        CHECK(t.dim(0) == 3);
        CHECK(t.dim(1) == 4);
        CHECK(t.dim(2) == 5);

        CHECK(t.size() == 60);
        CHECK_FALSE(t.empty());
    }

    TEST_CASE("iterators") {
        mat::Tensor<int, 2, 2, 2> t;
        t.fill(5);

        int sum = 0;
        for (auto val : t) {
            sum += val;
        }
        CHECK(sum == 40); // 8 elements * 5
    }

    TEST_CASE("operations") {
        mat::Tensor<double, 2, 2, 2> t;
        t.fill(3.14);

        CHECK(t(0, 0, 0) == doctest::Approx(3.14));
        CHECK(t(1, 1, 1) == doctest::Approx(3.14));

        mat::Tensor<int, 2, 2, 2> a;
        mat::Tensor<int, 2, 2, 2> b;
        a.fill(1);
        b.fill(2);
        a.swap(b);

        CHECK(a(0, 0, 0) == 2);
        CHECK(b(0, 0, 0) == 1);
    }

    TEST_CASE("comparison") {
        mat::Tensor<int, 2, 2, 2> a;
        mat::Tensor<int, 2, 2, 2> b;
        mat::Tensor<int, 2, 2, 2> c;

        a.fill(1);
        b.fill(1);
        c.fill(2);

        CHECK(a == b);
        CHECK(a != c);
    }

    TEST_CASE("reflection") {
        mat::Tensor<double, 2, 2, 2> t;
        t.fill(1.5);

        auto tuple = t.members();
        auto &arr = std::get<0>(tuple);
        CHECK(arr[0] == 1.5);

        auto t2 = to_tuple(t);
        auto &arr2 = std::get<0>(t2);
        CHECK(arr2[0] == 1.5);
    }

    TEST_CASE("type traits") {
        CHECK(mat::is_tensor_v<mat::Tensor<double, 2, 2, 2>>);
        CHECK(mat::is_tensor_v<mat::Tensor<float, 3, 3, 3>>);
        CHECK_FALSE(mat::is_tensor_v<double>);
    }

    TEST_CASE("POD compatibility") {
        CHECK(std::is_trivially_copyable_v<mat::Tensor<double, 2, 2, 2>>);
        CHECK(std::is_trivially_copyable_v<mat::Tensor<int, 3, 3, 3>>);
    }

    TEST_CASE("type aliases") {
        static_assert(std::is_same_v<mat::tensor3d_2x2x2d, mat::Tensor<double, 2, 2, 2>>);
        static_assert(std::is_same_v<mat::tensor3d_3x3x3f, mat::Tensor<float, 3, 3, 3>>);
    }

    TEST_CASE("alignment") {
        mat::Tensor<double, 2, 2, 2> t;
        CHECK(reinterpret_cast<uintptr_t>(t.data()) % 32 == 0);
    }

    TEST_CASE("4D tensor") {
        mat::Tensor<double, 2, 2, 2, 2> t;
        t.fill(0.0);

        CHECK(mat::Tensor<double, 2, 2, 2, 2>::rank == 4);
        CHECK(t.size() == 16);

        t(0, 0, 0, 0) = 1.0;
        t(1, 1, 1, 1) = 2.0;

        CHECK(t(0, 0, 0, 0) == 1.0);
        CHECK(t(1, 1, 1, 1) == 2.0);
    }

    TEST_CASE("column-major layout") {
        mat::Tensor<int, 2, 2, 2> t;
        // In column-major order, first index changes fastest
        int val = 0;
        for (size_t k = 0; k < 2; ++k) {
            for (size_t j = 0; j < 2; ++j) {
                for (size_t i = 0; i < 2; ++i) {
                    t(i, j, k) = val++;
                }
            }
        }

        // Linear access should match column-major order
        for (size_t i = 0; i < 8; ++i) {
            CHECK(t[i] == static_cast<int>(i));
        }
    }

    TEST_CASE("common use cases") {
        // 3D voxel grid
        mat::Tensor<float, 16, 16, 16> voxels;
        voxels.fill(0.0f);
        CHECK(voxels.size() == 4096);

        // RGB image (small)
        mat::Tensor<uint8_t, 4, 4, 3> image;
        image.fill(255);
        CHECK(image.size() == 48);

        // 4D batch of images
        mat::Tensor<float, 2, 3, 4, 4> batch; // batch x channels x height x width
        batch.fill(1.0f);
        CHECK(batch.size() == 96);
    }
}
