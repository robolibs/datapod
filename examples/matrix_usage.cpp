#include <datapod/matrix/matrix.hpp>
#include <iomanip>
#include <iostream>

using namespace datapod;

void print_matrix(const matrix<double, 3, 3> &m, const std::string &name) {
    std::cout << "   " << name << ":" << std::endl;
    for (size_t i = 0; i < 3; ++i) {
        std::cout << "   ";
        for (size_t j = 0; j < 3; ++j) {
            std::cout << std::setw(8) << std::fixed << std::setprecision(2) << m(i, j) << " ";
        }
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "=== matrix Usage Examples ===" << std::endl << std::endl;

    // 1. Construction and element access
    std::cout << "1. Construction and 2D Indexing:" << std::endl;
    matrix<double, 3, 3> m;
    m(0, 0) = 1.0;
    m(0, 1) = 2.0;
    m(0, 2) = 3.0;
    m(1, 0) = 4.0;
    m(1, 1) = 5.0;
    m(1, 2) = 6.0;
    m(2, 0) = 7.0;
    m(2, 1) = 8.0;
    m(2, 2) = 9.0;

    std::cout << "   m(0,0) = " << m(0, 0) << std::endl;
    std::cout << "   m(1,1) = " << m(1, 1) << std::endl;
    std::cout << "   m(2,2) = " << m(2, 2) << std::endl << std::endl;

    // 2. Identity matrix
    std::cout << "2. Identity Matrix:" << std::endl;
    matrix<double, 3, 3> identity;
    identity.set_identity();
    print_matrix(identity, "Identity");
    std::cout << std::endl;

    // 3. Fill operation
    std::cout << "3. Fill Operation:" << std::endl;
    matrix<double, 3, 3> filled;
    filled.fill(2.5);
    std::cout << "   filled(0,0) = " << filled(0, 0) << std::endl;
    std::cout << "   filled(2,2) = " << filled(2, 2) << std::endl << std::endl;

    // 4. Dimensions
    std::cout << "4. Dimensions:" << std::endl;
    matrix<double, 4, 4> big_matrix;
    std::cout << "   rows = " << big_matrix.rows() << std::endl;
    std::cout << "   cols = " << big_matrix.cols() << std::endl;
    std::cout << "   size = " << big_matrix.size() << " elements" << std::endl << std::endl;

    // 5. Iteration
    std::cout << "5. Linear Iteration (column-major):" << std::endl;
    matrix<int, 2, 2> mat;
    mat(0, 0) = 1;
    mat(0, 1) = 3;
    mat(1, 0) = 2;
    mat(1, 1) = 4;

    std::cout << "   Matrix layout:" << std::endl;
    std::cout << "      " << mat(0, 0) << " " << mat(0, 1) << std::endl;
    std::cout << "      " << mat(1, 0) << " " << mat(1, 1) << std::endl;

    std::cout << "   Linear access (column-major): ";
    for (size_t i = 0; i < 4; ++i) {
        std::cout << mat[i] << " ";
    }
    std::cout << std::endl << std::endl;

    // 6. Type traits
    std::cout << "6. Type Traits:" << std::endl;
    std::cout << "   is_matrix_v<matrix<double,3,3>>: " << is_matrix_v<matrix<double, 3, 3>> << std::endl;
    std::cout << "   rank: " << matrix<double, 3, 3>::rank << " (rank-2 tensor)" << std::endl;
    std::cout << "   rows: " << matrix<double, 3, 3>::rows_ << std::endl;
    std::cout << "   cols: " << matrix<double, 3, 3>::cols_ << std::endl << std::endl;

    // 7. Common use cases
    std::cout << "7. Common Use Cases:" << std::endl;

    // Rotation matrix (SO(3))
    matrix3x3d rotation;
    rotation.set_identity();
    std::cout << "   3x3 Rotation matrix (identity):" << std::endl;
    std::cout << "      R(0,0) = " << rotation(0, 0) << std::endl;

    // Transformation matrix (SE(3))
    matrix4x4d transform;
    transform.set_identity();
    std::cout << "   4x4 Transform matrix (identity):" << std::endl;
    std::cout << "      T(3,3) = " << transform(3, 3) << std::endl;

    // Covariance matrix
    matrix6x6d covariance;
    covariance.fill(0.0);
    covariance.set_identity(); // Make it identity for now
    std::cout << "   6x6 Covariance matrix:" << std::endl;
    std::cout << "      Σ(0,0) = " << covariance(0, 0) << std::endl;

    return 0;
}
