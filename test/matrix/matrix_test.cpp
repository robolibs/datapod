#include <doctest/doctest.h>

#include "datapod/matrix/matrix.hpp"
#include "datapod/matrix/scalar.hpp"
#include "datapod/reflection/for_each_field.hpp"
#include "datapod/reflection/to_tuple.hpp"

using namespace datapod;
using namespace datapod::mat;

TEST_SUITE("mat::matrix") {
    TEST_CASE("construction") {
        matrix<double, 3, 3> m;
        m(0, 0) = 1.0;
        m(1, 1) = 2.0;
        m(2, 2) = 3.0;

        CHECK(m(0, 0) == 1.0);
        CHECK(m(1, 1) == 2.0);
        CHECK(m(2, 2) == 3.0);
    }

    TEST_CASE("matrix with scalar type") {
        matrix<scalar<double>, 2, 2> m;
        m(0, 0) = scalar<double>{1.0};
        m(0, 1) = scalar<double>{2.0};
        m(1, 0) = scalar<double>{3.0};
        m(1, 1) = scalar<double>{4.0};

        CHECK(m(0, 0).value == 1.0);
        CHECK(m(1, 1).value == 4.0);
    }

    TEST_CASE("element access 2D") {
        matrix<double, 2, 2> m;
        m(0, 0) = 1.0;
        m(0, 1) = 2.0;
        m(1, 0) = 4.0;
        m(1, 1) = 5.0;

        CHECK(m(0, 0) == 1.0);
        CHECK(m(0, 1) == 2.0);
        CHECK(m(1, 1) == 5.0);
        CHECK(m.at(1, 1) == 5.0);

        CHECK_THROWS_AS(m.at(2, 0), std::out_of_range);
        CHECK_THROWS_AS(m.at(0, 2), std::out_of_range);
    }

    TEST_CASE("dimensions") {
        matrix<double, 3, 3> m; // Changed to square matrix

        CHECK(m.rows() == 3);
        CHECK(m.cols() == 3);
        CHECK(m.size() == 9);
        CHECK_FALSE(m.empty());
        CHECK(matrix<double, 3, 3>::rank == 2);
    }

    TEST_CASE("iterators") {
        matrix<int, 2, 2> m;
        m(0, 0) = 1;
        m(0, 1) = 2;
        m(1, 0) = 3;
        m(1, 1) = 4;

        int sum = 0;
        for (auto val : m) {
            sum += val;
        }
        CHECK(sum == 10);
    }

    TEST_CASE("operations") {
        matrix<double, 2, 2> m;
        m.fill(5.0);
        CHECK(m(0, 0) == 5.0);
        CHECK(m(1, 1) == 5.0);

        matrix<int, 2, 2> a;
        a(0, 0) = 1;
        a(0, 1) = 2;
        a(1, 0) = 3;
        a(1, 1) = 4;

        matrix<int, 2, 2> b;
        b(0, 0) = 10;
        b(0, 1) = 20;
        b(1, 0) = 30;
        b(1, 1) = 40;

        a.swap(b);
        CHECK(a(0, 0) == 10);
        CHECK(b(0, 0) == 1);
    }

    TEST_CASE("set identity") {
        matrix<double, 3, 3> m;
        m.fill(0.0);
        m.set_identity();

        CHECK(m(0, 0) == 1.0);
        CHECK(m(1, 1) == 1.0);
        CHECK(m(2, 2) == 1.0);
        CHECK(m(0, 1) == 0.0);
        CHECK(m(1, 0) == 0.0);
    }

    TEST_CASE("comparison") {
        matrix<int, 2, 2> a;
        a(0, 0) = 1;
        a(0, 1) = 2;
        a(1, 0) = 3;
        a(1, 1) = 4;

        matrix<int, 2, 2> b;
        b(0, 0) = 1;
        b(0, 1) = 2;
        b(1, 0) = 3;
        b(1, 1) = 4;

        matrix<int, 2, 2> c;
        c(0, 0) = 1;
        c(0, 1) = 2;
        c(1, 0) = 3;
        c(1, 1) = 5; // Different

        CHECK(a == b);
        CHECK(a != c);
    }

    TEST_CASE("reflection") {
        matrix<double, 2, 2> m;
        m(0, 0) = 1.0;
        m(0, 1) = 2.0;
        m(1, 0) = 3.0;
        m(1, 1) = 4.0;

        auto tuple = m.members();
        auto &arr = std::get<0>(tuple);
        CHECK(arr[0] == 1.0); // Column-major: m(0,0)

        auto t2 = to_tuple(m);
        auto &arr2 = std::get<0>(t2);
        CHECK(arr2[0] == 1.0);
    }

    TEST_CASE("type traits") {
        CHECK(is_matrix_v<matrix<double, 3, 3>>);
        CHECK_FALSE(is_matrix_v<double>);
    }

    TEST_CASE("POD compatibility") {
        CHECK(std::is_trivially_copyable_v<matrix<double, 3, 3>>);
        CHECK(std::is_trivially_copyable_v<matrix<int, 4, 4>>);
    }

    TEST_CASE("type aliases") {
        static_assert(std::is_same_v<matrix3x3d, matrix<double, 3, 3>>);
        static_assert(std::is_same_v<matrix4x4f, matrix<float, 4, 4>>);
    }

    TEST_CASE("alignment") {
        matrix<double, 3, 3> m;
        CHECK(reinterpret_cast<uintptr_t>(m.data()) % 32 == 0);
    }

    TEST_CASE("column-major layout") {
        matrix<int, 2, 2> m;
        // Fill column by column
        m(0, 0) = 1;
        m(1, 0) = 2; // Column 0
        m(0, 1) = 3;
        m(1, 1) = 4; // Column 1

        // Linear access should be column-major
        CHECK(m[0] == 1); // (0,0)
        CHECK(m[1] == 2); // (1,0)
        CHECK(m[2] == 3); // (0,1)
        CHECK(m[3] == 4); // (1,1)
    }

    TEST_CASE("common use cases") {
        matrix<double, 3, 3> rotation;
        rotation.set_identity();
        CHECK(rotation(0, 0) == 1.0);

        matrix<double, 4, 4> transform;
        transform.fill(0.0);
        transform.set_identity();
        CHECK(transform(3, 3) == 1.0);

        matrix<double, 6, 6> covariance;
        covariance.fill(0.0);
        CHECK(covariance.size() == 36);
    }
}
