#include <datapod/pods/matrix/matrix.hpp>
#include <datapod/pods/matrix/scalar.hpp>
#include <datapod/pods/matrix/tensor.hpp>
#include <datapod/pods/matrix/vector.hpp>

#include <cassert>
#include <iostream>

// Not using datapod::mat to avoid Vector conflict
using namespace datapod;

// Test 1: Matrix composed from vectors (as columns)
void test_matrix_from_vectors() {
    std::cout << "Test 1: Matrix composition from vectors (columns)..." << std::endl;

    // Create column vectors
    mat::Vector<double, 3> col0{1.0, 2.0, 3.0};
    mat::Vector<double, 3> col1{4.0, 5.0, 6.0};
    mat::Vector<double, 3> col2{7.0, 8.0, 9.0};

    // Compose matrix from vectors (each vector becomes a column)
    mat::Matrix<double, 3, 3> m(col0, col1, col2);

    // Verify column-major layout
    assert(m(0, 0) == 1.0);
    assert(m(1, 0) == 2.0);
    assert(m(2, 0) == 3.0);
    assert(m(0, 1) == 4.0);
    assert(m(1, 1) == 5.0);
    assert(m(2, 1) == 6.0);
    assert(m(0, 2) == 7.0);
    assert(m(1, 2) == 8.0);
    assert(m(2, 2) == 9.0);

    std::cout << "  ✓ Matrix from vectors works!" << std::endl;
    std::cout << "    Matrix layout:" << std::endl;
    std::cout << "    [" << m(0, 0) << " " << m(0, 1) << " " << m(0, 2) << "]" << std::endl;
    std::cout << "    [" << m(1, 0) << " " << m(1, 1) << " " << m(1, 2) << "]" << std::endl;
    std::cout << "    [" << m(2, 0) << " " << m(2, 1) << " " << m(2, 2) << "]" << std::endl;
}

// Test 2: Matrix from vectors with mat::Scalar<T>
void test_matrix_from_scalar_vectors() {
    std::cout << "\nTest 2: Matrix from vectors of mat::Scalar<T>..." << std::endl;

    mat::Vector<mat::Scalar<float>, 2> v0;
    v0[0] = mat::Scalar<float>{1.0f};
    v0[1] = mat::Scalar<float>{2.0f};

    mat::Vector<mat::Scalar<float>, 2> v1;
    v1[0] = mat::Scalar<float>{3.0f};
    v1[1] = mat::Scalar<float>{4.0f};

    mat::Matrix<mat::Scalar<float>, 2, 2> m(v0, v1);

    assert(m(0, 0).value == 1.0f);
    assert(m(1, 0).value == 2.0f);
    assert(m(0, 1).value == 3.0f);
    assert(m(1, 1).value == 4.0f);

    std::cout << "  ✓ Matrix from mat::Scalar<T> vectors works!" << std::endl;
}

// Test 3: Tensor composed from matrices (as slices)
void test_tensor_from_matrices() {
    std::cout << "\nTest 3: Tensor composition from matrices (slices)..." << std::endl;

    // Create two 2x2 matrices
    mat::Matrix<double, 2, 2> mat0;
    mat0(0, 0) = 1.0;
    mat0(0, 1) = 2.0;
    mat0(1, 0) = 3.0;
    mat0(1, 1) = 4.0;

    mat::Matrix<double, 2, 2> mat1;
    mat1(0, 0) = 5.0;
    mat1(0, 1) = 6.0;
    mat1(1, 0) = 7.0;
    mat1(1, 1) = 8.0;

    // Compose tensor from matrices (each matrix becomes a slice)
    mat::Tensor<double, 2, 2, 2> t(mat0, mat1);

    // Verify first slice (mat0)
    assert(t(0, 0, 0) == 1.0);
    assert(t(0, 1, 0) == 2.0);
    assert(t(1, 0, 0) == 3.0);
    assert(t(1, 1, 0) == 4.0);

    // Verify second slice (mat1)
    assert(t(0, 0, 1) == 5.0);
    assert(t(0, 1, 1) == 6.0);
    assert(t(1, 0, 1) == 7.0);
    assert(t(1, 1, 1) == 8.0);

    std::cout << "  ✓ Tensor from matrices works!" << std::endl;
    std::cout << "    Slice 0:" << std::endl;
    std::cout << "    [" << t(0, 0, 0) << " " << t(0, 1, 0) << "]" << std::endl;
    std::cout << "    [" << t(1, 0, 0) << " " << t(1, 1, 0) << "]" << std::endl;
    std::cout << "    Slice 1:" << std::endl;
    std::cout << "    [" << t(0, 0, 1) << " " << t(0, 1, 1) << "]" << std::endl;
    std::cout << "    [" << t(1, 0, 1) << " " << t(1, 1, 1) << "]" << std::endl;
}

// Test 4: Tensor from 3 matrices (3D cube)
void test_tensor_from_three_matrices() {
    std::cout << "\nTest 4: 3D Tensor from three 2x2 matrices..." << std::endl;

    mat::Matrix<int, 2, 2> m0, m1, m2;
    m0(0, 0) = 1;
    m0(1, 0) = 2;
    m0(0, 1) = 3;
    m0(1, 1) = 4;
    m1(0, 0) = 5;
    m1(1, 0) = 6;
    m1(0, 1) = 7;
    m1(1, 1) = 8;
    m2(0, 0) = 9;
    m2(1, 0) = 10;
    m2(0, 1) = 11;
    m2(1, 1) = 12;

    mat::Tensor<int, 2, 2, 3> t(m0, m1, m2);

    // Verify all slices
    assert(t(0, 0, 0) == 1);
    assert(t(1, 0, 0) == 2);
    assert(t(0, 1, 0) == 3);
    assert(t(1, 1, 0) == 4);

    assert(t(0, 0, 1) == 5);
    assert(t(1, 0, 1) == 6);
    assert(t(0, 1, 1) == 7);
    assert(t(1, 1, 1) == 8);

    assert(t(0, 0, 2) == 9);
    assert(t(1, 0, 2) == 10);
    assert(t(0, 1, 2) == 11);
    assert(t(1, 1, 2) == 12);

    std::cout << "  ✓ Tensor from 3 matrices works!" << std::endl;
}

// Test 5: Layered composition (vectors -> matrix -> tensor)
void test_layered_composition() {
    std::cout << "\nTest 5: Layered composition (vectors -> matrix -> tensor)..." << std::endl;

    // Build from the ground up
    // Step 1: Create vectors
    mat::Vector<double, 2> v0{1.0, 2.0};
    mat::Vector<double, 2> v1{3.0, 4.0};
    mat::Vector<double, 2> v2{5.0, 6.0};
    mat::Vector<double, 2> v3{7.0, 8.0};

    // Step 2: Compose matrices from vectors
    mat::Matrix<double, 2, 2> mat0(v0, v1);
    mat::Matrix<double, 2, 2> mat1(v2, v3);

    // Step 3: Compose tensor from matrices
    mat::Tensor<double, 2, 2, 2> t(mat0, mat1);

    // Verify the entire structure
    assert(t(0, 0, 0) == 1.0);
    assert(t(1, 0, 0) == 2.0);
    assert(t(0, 1, 0) == 3.0);
    assert(t(1, 1, 0) == 4.0);
    assert(t(0, 0, 1) == 5.0);
    assert(t(1, 0, 1) == 6.0);
    assert(t(0, 1, 1) == 7.0);
    assert(t(1, 1, 1) == 8.0);

    std::cout << "  ✓ Layered composition works!" << std::endl;
    std::cout << "    vectors -> matrix -> tensor composition successful!" << std::endl;
}

// Test 6: Column-major memory verification
void test_column_major_consistency() {
    std::cout << "\nTest 6: Column-major memory layout verification..." << std::endl;

    // Matrix from vectors
    mat::Vector<int, 3> c0{1, 2, 3};
    mat::Vector<int, 3> c1{4, 5, 6};
    mat::Matrix<int, 3, 2> m(c0, c1);

    // In column-major: [1, 2, 3, 4, 5, 6] (col0 then col1)
    const int *data = m.data();
    assert(data[0] == 1 && data[1] == 2 && data[2] == 3);
    assert(data[3] == 4 && data[4] == 5 && data[5] == 6);

    // Tensor from matrices
    mat::Matrix<int, 2, 2> m0;
    m0(0, 0) = 1;
    m0(1, 0) = 2;
    m0(0, 1) = 3;
    m0(1, 1) = 4;
    mat::Matrix<int, 2, 2> m1;
    m1(0, 0) = 5;
    m1(1, 0) = 6;
    m1(0, 1) = 7;
    m1(1, 1) = 8;
    mat::Tensor<int, 2, 2, 2> t(m0, m1);

    // Verify tensor memory layout (column-major: first dim varies fastest)
    const int *tdata = t.data();
    assert(tdata[0] == 1); // (0,0,0)
    assert(tdata[1] == 2); // (1,0,0)
    assert(tdata[2] == 3); // (0,1,0)
    assert(tdata[3] == 4); // (1,1,0)
    assert(tdata[4] == 5); // (0,0,1)
    assert(tdata[5] == 6); // (1,0,1)
    assert(tdata[6] == 7); // (0,1,1)
    assert(tdata[7] == 8); // (1,1,1)

    std::cout << "  ✓ Column-major layout is consistent!" << std::endl;
}

// Test 7: Working with different numeric types
void test_different_types() {
    std::cout << "\nTest 7: Composition with different numeric types..." << std::endl;

    // Float vectors -> float matrix
    mat::Vector<float, 2> vf0{1.5f, 2.5f};
    mat::Vector<float, 2> vf1{3.5f, 4.5f};
    mat::Matrix<float, 2, 2> mf(vf0, vf1);
    assert(mf(0, 0) == 1.5f);
    assert(mf(1, 1) == 4.5f);

    // Int matrices -> int tensor
    mat::Matrix<int, 2, 2> mi0;
    mi0(0, 0) = 10;
    mi0(1, 0) = 20;
    mi0(0, 1) = 30;
    mi0(1, 1) = 40;
    mat::Matrix<int, 2, 2> mi1;
    mi1(0, 0) = 50;
    mi1(1, 0) = 60;
    mi1(0, 1) = 70;
    mi1(1, 1) = 80;
    mat::Tensor<int, 2, 2, 2> ti(mi0, mi1);
    assert(ti(0, 0, 0) == 10);
    assert(ti(1, 1, 1) == 80);

    std::cout << "  ✓ Different numeric types work!" << std::endl;
}

int main() {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "Matrix Composition Tests - Hierarchical Construction" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    test_matrix_from_vectors();
    test_matrix_from_scalar_vectors();
    test_tensor_from_matrices();
    test_tensor_from_three_matrices();
    test_layered_composition();
    test_column_major_consistency();
    test_different_types();

    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "✓ All composition tests passed!" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << std::endl;

    return 0;
}
