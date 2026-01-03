#include <datapod/datapod.hpp>

#include <doctest/doctest.h>

using namespace datapod;
// Not using datapod::mat to avoid Vector conflict

// =============================================================================
// DYNAMIC VECTOR TESTS
// =============================================================================

TEST_CASE("mat::Vector<T, mat::Dynamic>: construction") {
    SUBCASE("default constructor") {
        mat::Vector<double, mat::Dynamic> v;
        CHECK(v.size() == 0);
        CHECK(v.empty());
    }

    SUBCASE("size constructor") {
        mat::Vector<double, mat::Dynamic> v(100);
        CHECK(v.size() == 100);
        CHECK(!v.empty());
        // Check zero-initialized
        for (size_t i = 0; i < v.size(); ++i) {
            CHECK(v[i] == 0.0);
        }
    }

    SUBCASE("size + value constructor") {
        mat::Vector<int, mat::Dynamic> v(50, 42);
        CHECK(v.size() == 50);
        for (size_t i = 0; i < v.size(); ++i) {
            CHECK(v[i] == 42);
        }
    }

    SUBCASE("initializer list constructor") {
        mat::Vector<double, mat::Dynamic> v = {1.0, 2.0, 3.0, 4.0, 5.0};
        CHECK(v.size() == 5);
        CHECK(v[0] == 1.0);
        CHECK(v[1] == 2.0);
        CHECK(v[4] == 5.0);
    }
}

TEST_CASE("mat::Vector<T, mat::Dynamic>: element access") {
    mat::Vector<double, mat::Dynamic> v = {10.0, 20.0, 30.0};

    SUBCASE("operator[]") {
        CHECK(v[0] == 10.0);
        CHECK(v[1] == 20.0);
        v[1] = 25.0;
        CHECK(v[1] == 25.0);
    }

    SUBCASE("at() with bounds checking") {
        CHECK(v.at(0) == 10.0);
        CHECK_THROWS_AS(v.at(10), std::out_of_range);
    }

    SUBCASE("front() and back()") {
        CHECK(v.front() == 10.0);
        CHECK(v.back() == 30.0);
    }
}

TEST_CASE("mat::Vector<T, mat::Dynamic>: resize and capacity") {
    mat::Vector<double, mat::Dynamic> v(10);

    SUBCASE("resize smaller") {
        v.resize(5);
        CHECK(v.size() == 5);
    }

    SUBCASE("resize larger") {
        v.resize(20);
        CHECK(v.size() == 20);
    }

    SUBCASE("reserve") {
        v.reserve(100);
        CHECK(v.capacity() >= 100);
        CHECK(v.size() == 10);
    }

    SUBCASE("clear") {
        v.clear();
        CHECK(v.size() == 0);
        CHECK(v.empty());
    }

    SUBCASE("push_back and pop_back") {
        v.clear();
        v.push_back(1.0);
        v.push_back(2.0);
        v.push_back(3.0);
        CHECK(v.size() == 3);
        CHECK(v.back() == 3.0);

        v.pop_back();
        CHECK(v.size() == 2);
        CHECK(v.back() == 2.0);
    }
}

TEST_CASE("mat::Vector<T, mat::Dynamic>: copy and move") {
    mat::Vector<double, mat::Dynamic> v1 = {1.0, 2.0, 3.0};

    SUBCASE("copy constructor") {
        mat::Vector<double, mat::Dynamic> v2(v1);
        CHECK(v2.size() == 3);
        CHECK(v2[0] == 1.0);
        CHECK(v2[2] == 3.0);
        // Modify original, copy should be unaffected
        v1[0] = 100.0;
        CHECK(v2[0] == 1.0);
    }

    SUBCASE("move constructor") {
        mat::Vector<double, mat::Dynamic> v2(std::move(v1));
        CHECK(v2.size() == 3);
        CHECK(v2[0] == 1.0);
        CHECK(v1.size() == 0); // Moved-from state
    }

    SUBCASE("copy assignment") {
        mat::Vector<double, mat::Dynamic> v2;
        v2 = v1;
        CHECK(v2.size() == 3);
    }

    SUBCASE("move assignment") {
        mat::Vector<double, mat::Dynamic> v2;
        v2 = std::move(v1);
        CHECK(v2.size() == 3);
        CHECK(v1.size() == 0);
    }
}

TEST_CASE("mat::Vector<T, mat::Dynamic>: comparison") {
    mat::Vector<double, mat::Dynamic> v1 = {1.0, 2.0, 3.0};
    mat::Vector<double, mat::Dynamic> v2 = {1.0, 2.0, 3.0};
    mat::Vector<double, mat::Dynamic> v3 = {1.0, 2.0, 4.0};
    mat::Vector<double, mat::Dynamic> v4 = {1.0, 2.0};

    CHECK(v1 == v2);
    CHECK(v1 != v3);
    CHECK(v1 != v4);
}

TEST_CASE("mat::Vector<T, mat::Dynamic>: Eigen-style aliases") {
    mat::VectorXd vd(10);
    mat::VectorXf vf(10);
    mat::VectorXi vi(10);

    CHECK(vd.size() == 10);
    CHECK(vf.size() == 10);
    CHECK(vi.size() == 10);
}

// =============================================================================
// DYNAMIC MATRIX TESTS
// =============================================================================

TEST_CASE("mat::Matrix<T, mat::Dynamic, mat::Dynamic>: construction") {
    SUBCASE("default constructor") {
        mat::Matrix<double, mat::Dynamic, mat::Dynamic> m;
        CHECK(m.rows() == 0);
        CHECK(m.cols() == 0);
        CHECK(m.empty());
    }

    SUBCASE("size constructor") {
        mat::Matrix<double, mat::Dynamic, mat::Dynamic> m(100, 50);
        CHECK(m.rows() == 100);
        CHECK(m.cols() == 50);
        CHECK(m.size() == 5000);
        // Check zero-initialized
        for (size_t i = 0; i < m.rows(); ++i) {
            for (size_t j = 0; j < m.cols(); ++j) {
                CHECK(m(i, j) == 0.0);
            }
        }
    }

    SUBCASE("size + value constructor") {
        mat::Matrix<int, mat::Dynamic, mat::Dynamic> m(10, 10, 7);
        CHECK(m.rows() == 10);
        CHECK(m.cols() == 10);
        for (size_t i = 0; i < m.rows(); ++i) {
            for (size_t j = 0; j < m.cols(); ++j) {
                CHECK(m(i, j) == 7);
            }
        }
    }

    SUBCASE("initializer list constructor") {
        // 2x3 matrix, column-major
        mat::Matrix<double, mat::Dynamic, mat::Dynamic> m(2, 3, {1, 2, 3, 4, 5, 6});
        CHECK(m.rows() == 2);
        CHECK(m.cols() == 3);
        // Column-major: first column is {1,2}, second is {3,4}, third is {5,6}
        CHECK(m(0, 0) == 1);
        CHECK(m(1, 0) == 2);
        CHECK(m(0, 1) == 3);
        CHECK(m(1, 1) == 4);
    }
}

TEST_CASE("mat::Matrix<T, mat::Dynamic, mat::Dynamic>: element access") {
    mat::Matrix<double, mat::Dynamic, mat::Dynamic> m(3, 3);
    m(0, 0) = 1.0;
    m(1, 1) = 2.0;
    m(2, 2) = 3.0;

    SUBCASE("operator()") {
        CHECK(m(0, 0) == 1.0);
        CHECK(m(1, 1) == 2.0);
        CHECK(m(2, 2) == 3.0);
    }

    SUBCASE("at() with bounds checking") {
        CHECK(m.at(0, 0) == 1.0);
        CHECK_THROWS_AS(m.at(10, 0), std::out_of_range);
        CHECK_THROWS_AS(m.at(0, 10), std::out_of_range);
    }

    SUBCASE("linear indexing") {
        // Column-major: m[0] = m(0,0), m[1] = m(1,0), m[3] = m(0,1)
        CHECK(m[0] == 1.0);
        CHECK(m[4] == 2.0); // m(1,1) in column-major
        CHECK(m[8] == 3.0); // m(2,2) in column-major
    }
}

TEST_CASE("mat::Matrix<T, mat::Dynamic, mat::Dynamic>: resize") {
    mat::Matrix<double, mat::Dynamic, mat::Dynamic> m(10, 10);
    m(5, 5) = 42.0;

    SUBCASE("resize (destructive)") {
        m.resize(20, 20);
        CHECK(m.rows() == 20);
        CHECK(m.cols() == 20);
        // Data is not preserved
    }

    SUBCASE("conservativeResize") {
        m.conservativeResize(20, 20);
        CHECK(m.rows() == 20);
        CHECK(m.cols() == 20);
        CHECK(m(5, 5) == 42.0); // Original data preserved
    }
}

TEST_CASE("mat::Matrix<T, mat::Dynamic, mat::Dynamic>: operations") {
    SUBCASE("setIdentity") {
        mat::Matrix<double, mat::Dynamic, mat::Dynamic> m(4, 4);
        m.setIdentity();
        for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 4; ++j) {
                if (i == j) {
                    CHECK(m(i, j) == 1.0);
                } else {
                    CHECK(m(i, j) == 0.0);
                }
            }
        }
    }

    SUBCASE("setZero") {
        mat::Matrix<double, mat::Dynamic, mat::Dynamic> m(3, 3, 5.0);
        m.setZero();
        for (size_t i = 0; i < 9; ++i) {
            CHECK(m[i] == 0.0);
        }
    }

    SUBCASE("fill") {
        mat::Matrix<double, mat::Dynamic, mat::Dynamic> m(3, 3);
        m.fill(7.5);
        for (size_t i = 0; i < 9; ++i) {
            CHECK(m[i] == 7.5);
        }
    }
}

TEST_CASE("mat::Matrix<T, mat::Dynamic, mat::Dynamic>: Eigen-style aliases") {
    mat::MatrixXd md(10, 20);
    mat::MatrixXf mf(10, 20);
    mat::MatrixXi mi(10, 20);

    CHECK(md.rows() == 10);
    CHECK(md.cols() == 20);
    CHECK(mf.rows() == 10);
    CHECK(mi.rows() == 10);
}

// =============================================================================
// DYNAMIC TENSOR TESTS
// =============================================================================

TEST_CASE("dynamic_tensor: construction") {
    SUBCASE("default constructor") {
        mat::DynamicTensor<double> t;
        CHECK(t.rank() == 0);
        CHECK(t.size() == 0);
        CHECK(t.empty());
    }

    SUBCASE("shape constructor (initializer_list)") {
        mat::DynamicTensor<double> t({10, 20, 30});
        CHECK(t.rank() == 3);
        CHECK(t.dim(0) == 10);
        CHECK(t.dim(1) == 20);
        CHECK(t.dim(2) == 30);
        CHECK(t.size() == 10 * 20 * 30);
    }

    SUBCASE("shape constructor (vector)") {
        datapod::Vector<size_t> shape;
        shape.push_back(5);
        shape.push_back(6);
        shape.push_back(7);
        shape.push_back(8);

        mat::DynamicTensor<double> t(shape);
        CHECK(t.rank() == 4);
        CHECK(t.size() == 5 * 6 * 7 * 8);
    }
}

TEST_CASE("dynamic_tensor: element access") {
    mat::DynamicTensor<double> t({4, 5, 6});

    SUBCASE("3D accessor") {
        t(1, 2, 3) = 42.0;
        CHECK(t(1, 2, 3) == 42.0);
    }

    SUBCASE("initializer_list accessor") {
        t({1, 2, 3}) = 99.0;
        CHECK(t({1, 2, 3}) == 99.0);
    }

    SUBCASE("linear indexing") {
        t[0] = 1.0;
        t[1] = 2.0;
        CHECK(t[0] == 1.0);
        CHECK(t[1] == 2.0);
    }

    SUBCASE("at() with bounds checking") {
        CHECK_NOTHROW(t.at({0, 0, 0}));
        CHECK_THROWS_AS(t.at({10, 0, 0}), std::out_of_range);
    }
}

TEST_CASE("dynamic_tensor: 4D tensor") {
    mat::DynamicTensor<double> t({2, 3, 4, 5});
    CHECK(t.rank() == 4);
    CHECK(t.size() == 2 * 3 * 4 * 5);

    t(1, 2, 3, 4) = 123.0;
    CHECK(t(1, 2, 3, 4) == 123.0);
}

TEST_CASE("dynamic_tensor: resize") {
    mat::DynamicTensor<double> t({10, 10, 10});
    CHECK(t.size() == 1000);

    t.resize({5, 5, 5, 5});
    CHECK(t.rank() == 4);
    CHECK(t.size() == 625);
}

TEST_CASE("dynamic_tensor: operations") {
    mat::DynamicTensor<double> t({3, 4, 5});

    SUBCASE("fill") {
        t.fill(3.14);
        for (size_t i = 0; i < t.size(); ++i) {
            CHECK(t[i] == 3.14);
        }
    }

    SUBCASE("setZero") {
        t.fill(1.0);
        t.setZero();
        for (size_t i = 0; i < t.size(); ++i) {
            CHECK(t[i] == 0.0);
        }
    }
}

TEST_CASE("dynamic_tensor: Eigen-style aliases") {
    mat::TensorXd td({10, 20, 30});
    mat::TensorXf tf({10, 20, 30});
    mat::TensorXi ti({10, 20, 30});

    CHECK(td.rank() == 3);
    CHECK(tf.rank() == 3);
    CHECK(ti.rank() == 3);
}

// =============================================================================
// PARTIALLY DYNAMIC TENSOR TESTS (mat::Tensor<T, mat::Dynamic, 4, 5>, etc.)
// =============================================================================

TEST_CASE("mat::Tensor<T, mat::Dynamic, ...>: fully dynamic construction") {
    SUBCASE("all dims dynamic - 3D") {
        mat::Tensor<double, mat::Dynamic, mat::Dynamic, mat::Dynamic> t(2, 3, 4);
        CHECK(t.dim(0) == 2);
        CHECK(t.dim(1) == 3);
        CHECK(t.dim(2) == 4);
        CHECK(t.size() == 24);
    }

    SUBCASE("all dims dynamic - 4D") {
        mat::Tensor<float, mat::Dynamic, mat::Dynamic, mat::Dynamic, mat::Dynamic> t(2, 3, 4, 5);
        CHECK(t.dim(0) == 2);
        CHECK(t.dim(1) == 3);
        CHECK(t.dim(2) == 4);
        CHECK(t.dim(3) == 5);
        CHECK(t.size() == 120);
    }
}

TEST_CASE("mat::Tensor<T, mat::Dynamic, ...>: mixed dynamic/fixed construction") {
    SUBCASE("first dim dynamic, others fixed") {
        // Batch of 4x5 matrices
        mat::Tensor<double, mat::Dynamic, 4, 5> batch(10);
        CHECK(batch.dim(0) == 10);
        CHECK(batch.dim(1) == 4);
        CHECK(batch.dim(2) == 5);
        CHECK(batch.size() == 200);
    }

    SUBCASE("middle dim dynamic") {
        mat::Tensor<double, 3, mat::Dynamic, 5> t(10);
        CHECK(t.dim(0) == 3);
        CHECK(t.dim(1) == 10);
        CHECK(t.dim(2) == 5);
        CHECK(t.size() == 150);
    }

    SUBCASE("last dim dynamic") {
        mat::Tensor<double, 3, 4, mat::Dynamic> t(10);
        CHECK(t.dim(0) == 3);
        CHECK(t.dim(1) == 4);
        CHECK(t.dim(2) == 10);
        CHECK(t.size() == 120);
    }

    SUBCASE("two dims dynamic") {
        mat::Tensor<double, mat::Dynamic, 4, mat::Dynamic> t(3, 5);
        CHECK(t.dim(0) == 3);
        CHECK(t.dim(1) == 4);
        CHECK(t.dim(2) == 5);
        CHECK(t.size() == 60);
    }
}

TEST_CASE("mat::Tensor<T, mat::Dynamic, ...>: element access") {
    mat::Tensor<double, mat::Dynamic, 4, 5> t(3);

    SUBCASE("write and read") {
        t(1, 2, 3) = 42.0;
        CHECK(t(1, 2, 3) == 42.0);
    }

    SUBCASE("linear indexing") {
        t[0] = 1.0;
        t[1] = 2.0;
        CHECK(t[0] == 1.0);
        CHECK(t[1] == 2.0);
    }

    SUBCASE("at() with bounds checking") {
        CHECK_NOTHROW(t.at(0, 0, 0));
        CHECK_THROWS_AS(t.at(10, 0, 0), std::out_of_range);
    }
}

TEST_CASE("mat::Tensor<T, mat::Dynamic, ...>: resize") {
    mat::Tensor<double, mat::Dynamic, 4, 5> t(3);
    CHECK(t.size() == 60);

    t.resize(10);
    CHECK(t.dim(0) == 10);
    CHECK(t.dim(1) == 4); // Fixed, unchanged
    CHECK(t.dim(2) == 5); // Fixed, unchanged
    CHECK(t.size() == 200);
}

TEST_CASE("mat::Tensor<T, mat::Dynamic, ...>: copy and move") {
    mat::Tensor<double, mat::Dynamic, mat::Dynamic, mat::Dynamic> t1(2, 3, 4);
    t1(0, 0, 0) = 1.0;
    t1(1, 2, 3) = 42.0;

    SUBCASE("copy constructor") {
        mat::Tensor<double, mat::Dynamic, mat::Dynamic, mat::Dynamic> t2(t1);
        CHECK(t2.dim(0) == 2);
        CHECK(t2.dim(1) == 3);
        CHECK(t2.dim(2) == 4);
        CHECK(t2(0, 0, 0) == 1.0);
        CHECK(t2(1, 2, 3) == 42.0);
    }

    SUBCASE("move constructor") {
        mat::Tensor<double, mat::Dynamic, mat::Dynamic, mat::Dynamic> t2(std::move(t1));
        CHECK(t2.dim(0) == 2);
        CHECK(t2(1, 2, 3) == 42.0);
        CHECK(t1.size() == 0); // Moved-from state
    }
}

TEST_CASE("mat::Tensor<T, mat::Dynamic, ...>: operations") {
    mat::Tensor<double, mat::Dynamic, 4, 5> t(3);

    SUBCASE("fill") {
        t.fill(3.14);
        for (size_t i = 0; i < t.size(); ++i) {
            CHECK(t[i] == 3.14);
        }
    }

    SUBCASE("setZero") {
        t.fill(1.0);
        t.setZero();
        for (size_t i = 0; i < t.size(); ++i) {
            CHECK(t[i] == 0.0);
        }
    }
}

TEST_CASE("mat::Tensor<T, mat::Dynamic, ...>: type traits") {
    CHECK(mat::is_partially_dynamic_tensor_v<mat::Tensor<double, mat::Dynamic, 4, 5>>);
    CHECK(mat::is_partially_dynamic_tensor_v<mat::Tensor<double, mat::Dynamic, mat::Dynamic, mat::Dynamic>>);
    CHECK(!mat::is_partially_dynamic_tensor_v<mat::Tensor<double, 3, 4, 5>>);

    // is_dynamic_v should include partially dynamic tensors
    CHECK(mat::is_dynamic_v<mat::Tensor<double, mat::Dynamic, 4, 5>>);
}

TEST_CASE("mat::Tensor<T, mat::Dynamic, ...>: serialization") {
    SUBCASE("fully dynamic 3D tensor") {
        mat::Tensor<double, mat::Dynamic, mat::Dynamic, mat::Dynamic> t1(2, 3, 4);
        for (size_t i = 0; i < t1.size(); ++i) {
            t1[i] = static_cast<double>(i);
        }

        auto buf = serialize(t1);
        auto t2 = deserialize<Mode::NONE, mat::Tensor<double, mat::Dynamic, mat::Dynamic, mat::Dynamic>>(buf);

        CHECK(t2.dim(0) == 2);
        CHECK(t2.dim(1) == 3);
        CHECK(t2.dim(2) == 4);
        CHECK(t1 == t2);
    }

    SUBCASE("partially dynamic tensor") {
        mat::Tensor<double, mat::Dynamic, 4, 5> t1(3);
        for (size_t i = 0; i < t1.size(); ++i) {
            t1[i] = static_cast<double>(i * 2);
        }

        auto buf = serialize(t1);
        auto t2 = deserialize<Mode::NONE, mat::Tensor<double, mat::Dynamic, 4, 5>>(buf);

        CHECK(t2.dim(0) == 3);
        CHECK(t2.dim(1) == 4);
        CHECK(t2.dim(2) == 5);
        CHECK(t1 == t2);
    }
}

// =============================================================================
// SERIALIZATION TESTS
// =============================================================================

TEST_CASE("mat::Vector<T, mat::Dynamic>: serialization") {
    mat::Vector<double, mat::Dynamic> v1 = {1.0, 2.0, 3.0, 4.0, 5.0};

    // Serialize
    auto buf = serialize(v1);

    // Deserialize
    auto v2 = deserialize<Mode::NONE, mat::Vector<double, mat::Dynamic>>(buf);

    CHECK(v2.size() == 5);
    CHECK(v2[0] == 1.0);
    CHECK(v2[1] == 2.0);
    CHECK(v2[4] == 5.0);
    CHECK(v1 == v2);
}

TEST_CASE("mat::Matrix<T, mat::Dynamic, mat::Dynamic>: serialization") {
    mat::Matrix<double, mat::Dynamic, mat::Dynamic> m1(3, 4);
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            m1(i, j) = static_cast<double>(i * 10 + j);
        }
    }

    // Serialize
    auto buf = serialize(m1);

    // Deserialize
    auto m2 = deserialize<Mode::NONE, mat::Matrix<double, mat::Dynamic, mat::Dynamic>>(buf);

    CHECK(m2.rows() == 3);
    CHECK(m2.cols() == 4);
    CHECK(m1 == m2);
}

TEST_CASE("dynamic_tensor: serialization") {
    mat::DynamicTensor<double> t1({2, 3, 4});
    for (size_t i = 0; i < t1.size(); ++i) {
        t1[i] = static_cast<double>(i);
    }

    // Serialize
    auto buf = serialize(t1);

    // Deserialize
    auto t2 = deserialize<Mode::NONE, mat::DynamicTensor<double>>(buf);

    CHECK(t2.rank() == 3);
    CHECK(t2.dim(0) == 2);
    CHECK(t2.dim(1) == 3);
    CHECK(t2.dim(2) == 4);
    CHECK(t1 == t2);
}

TEST_CASE("dynamic types: serialization with integrity check") {
    mat::Vector<double, mat::Dynamic> v = {1.0, 2.0, 3.0};

    auto buf = serialize<Mode::WITH_INTEGRITY>(v);
    auto v2 = deserialize<Mode::WITH_INTEGRITY, mat::Vector<double, mat::Dynamic>>(buf);

    CHECK(v == v2);
}

// =============================================================================
// TYPE TRAITS TESTS
// =============================================================================

TEST_CASE("type traits") {
    CHECK(mat::is_dynamic_vector_v<mat::Vector<double, mat::Dynamic>>);
    CHECK(!mat::is_dynamic_vector_v<mat::Vector<double, 3>>);

    CHECK(mat::is_dynamic_matrix_v<mat::Matrix<double, mat::Dynamic, mat::Dynamic>>);
    CHECK(!mat::is_dynamic_matrix_v<mat::Matrix<double, 3, 3>>);

    CHECK(mat::is_dynamic_tensor_v<mat::DynamicTensor<double>>);
    CHECK(!mat::is_dynamic_tensor_v<mat::Tensor<double, 2, 2, 2>>);

    CHECK(mat::is_dynamic_v<mat::Vector<double, mat::Dynamic>>);
    CHECK(mat::is_dynamic_v<mat::Matrix<double, mat::Dynamic, mat::Dynamic>>);
    CHECK(mat::is_dynamic_v<mat::DynamicTensor<double>>);
    CHECK(!mat::is_dynamic_v<mat::Vector<double, 3>>);
}

// =============================================================================
// DYNAMIC SENTINEL VALUE TEST
// =============================================================================

TEST_CASE("mat::Dynamic sentinel value") {
    CHECK(mat::Dynamic == static_cast<size_t>(-1));
    CHECK(mat::Dynamic > 1000000000); // Very large value
}
