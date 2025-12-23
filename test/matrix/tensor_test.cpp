#include <doctest/doctest.h>

#include "datapod/matrix/scalar.hpp"
#include "datapod/matrix/tensor.hpp"
#include "datapod/reflection/for_each_field.hpp"
#include "datapod/reflection/to_tuple.hpp"

using namespace datapod;

TEST_SUITE("tensor") {
    TEST_CASE("construction") {
        tensor<double, 3> t{1.0, 2.0, 3.0};
        CHECK(t[0] == 1.0);
        CHECK(t[1] == 2.0);
        CHECK(t[2] == 3.0);
    }

    TEST_CASE("deduction guide") {
        auto t = tensor{1.0, 2.0, 3.0};
        static_assert(std::is_same_v<decltype(t), tensor<double, 3>>);
        CHECK(t[0] == 1.0);
    }

    TEST_CASE("tensor with scalar type") {
        tensor<scalar<double>, 3> t;
        t[0] = scalar<double>{1.0};
        t[1] = scalar<double>{2.0};
        t[2] = scalar<double>{3.0};

        CHECK(t[0].value == 1.0);
        CHECK(t[1].value == 2.0);
        CHECK(t[2].value == 3.0);
    }

    TEST_CASE("element access") {
        tensor<double, 4> t{1.0, 2.0, 3.0, 4.0};

        CHECK(t[0] == 1.0);
        CHECK(t.at(3) == 4.0);
        CHECK(t.front() == 1.0);
        CHECK(t.back() == 4.0);

        CHECK_THROWS_AS(t.at(4), std::out_of_range);
    }

    TEST_CASE("capacity") {
        tensor<double, 6> t;

        CHECK(t.size() == 6);
        CHECK(t.length() == 6);
        CHECK_FALSE(t.empty());
        CHECK(tensor<double, 6>::rank == 1);
    }

    TEST_CASE("iterators") {
        tensor<int, 4> t{10, 20, 30, 40};

        int sum = 0;
        for (auto val : t) {
            sum += val;
        }
        CHECK(sum == 100);
    }

    TEST_CASE("operations") {
        tensor<double, 5> t;
        t.fill(7.0);
        for (size_t i = 0; i < 5; ++i) {
            CHECK(t[i] == 7.0);
        }

        tensor<int, 3> a{1, 2, 3};
        tensor<int, 3> b{10, 20, 30};
        a.swap(b);
        CHECK(a[0] == 10);
        CHECK(b[0] == 1);
    }

    TEST_CASE("comparison") {
        tensor<int, 3> a{1, 2, 3};
        tensor<int, 3> b{1, 2, 3};
        tensor<int, 3> c{1, 2, 4};

        CHECK(a == b);
        CHECK(a != c);
    }

    TEST_CASE("reflection") {
        tensor<double, 3> t{1.0, 2.0, 3.0};
        auto tuple = t.members();
        auto &arr = std::get<0>(tuple);
        CHECK(arr[0] == 1.0);

        auto t2 = to_tuple(t);
        auto &arr2 = std::get<0>(t2);
        CHECK(arr2[1] == 2.0);
    }

    TEST_CASE("type traits") {
        CHECK(is_tensor_v<tensor<double, 3>>);
        CHECK_FALSE(is_tensor_v<double>);
    }

    TEST_CASE("POD compatibility") {
        CHECK(std::is_trivially_copyable_v<tensor<double, 3>>);
        CHECK(std::is_trivially_copyable_v<tensor<int, 6>>);
    }

    TEST_CASE("type aliases") {
        static_assert(std::is_same_v<tensor3d, tensor<double, 3>>);
        static_assert(std::is_same_v<tensor6f, tensor<float, 6>>);
    }

    TEST_CASE("alignment") {
        tensor<double, 4> t;
        CHECK(reinterpret_cast<uintptr_t>(t.data()) % 32 == 0);
    }

    TEST_CASE("common use cases") {
        tensor<double, 3> position{1.0, 2.0, 3.0};
        CHECK(position.size() == 3);

        tensor<double, 6> state;
        state.fill(0.0);
        state[0] = 1.0;
        CHECK(state.size() == 6);
    }
}
