#include <cstdint>
#include <datapod/pods/matrix/tensor.hpp>
#include <iostream>

using namespace datapod::mat;

int main() {
    std::cout << "=== Tensor (N-dimensional) Usage Examples ===" << std::endl << std::endl;

    // 1. Construction
    std::cout << "1. Construction:" << std::endl;
    Tensor<double, 2, 3, 4> volume;
    volume.fill(0.0);
    std::cout << "   Created 2x3x4 tensor" << std::endl;
    std::cout << "   size = " << volume.size() << " elements" << std::endl;
    std::cout << "   rank = " << Tensor<double, 2, 3, 4>::rank << " dimensions" << std::endl << std::endl;

    // 2. Multi-dimensional indexing
    std::cout << "2. Multi-dimensional Indexing:" << std::endl;
    Tensor<int, 2, 2, 2> cube;
    cube.fill(0);
    cube(0, 0, 0) = 1;
    cube(1, 1, 1) = 8;
    cube(0, 1, 0) = 3;

    std::cout << "   cube(0,0,0) = " << cube(0, 0, 0) << std::endl;
    std::cout << "   cube(1,1,1) = " << cube(1, 1, 1) << std::endl;
    std::cout << "   cube(0,1,0) = " << cube(0, 1, 0) << std::endl << std::endl;

    // 3. Shape and dimensions
    std::cout << "3. Shape and Dimensions:" << std::endl;
    Tensor<float, 3, 4, 5> t;
    auto shape = t.shape();
    std::cout << "   shape = [" << shape[0] << ", " << shape[1] << ", " << shape[2] << "]" << std::endl;
    std::cout << "   dim(0) = " << t.dim(0) << std::endl;
    std::cout << "   dim(1) = " << t.dim(1) << std::endl;
    std::cout << "   dim(2) = " << t.dim(2) << std::endl << std::endl;

    // 4. Iteration
    std::cout << "4. Iteration:" << std::endl;
    Tensor<int, 2, 2, 2> small;
    for (size_t i = 0; i < 8; ++i) {
        small[i] = static_cast<int>(i + 1);
    }
    std::cout << "   Linear elements: ";
    for (auto val : small) {
        std::cout << val << " ";
    }
    std::cout << std::endl << std::endl;

    // 5. Type traits
    std::cout << "5. Type Traits:" << std::endl;
    std::cout << "   is_tensor_v<Tensor<double,2,2,2>>: " << is_tensor_v<Tensor<double, 2, 2, 2>> << std::endl;
    std::cout << "   rank: " << Tensor<double, 2, 2, 2>::rank << " (rank-3 tensor)" << std::endl << std::endl;

    // 6. 4D tensor
    std::cout << "6. 4D Tensor:" << std::endl;
    Tensor<double, 2, 2, 2, 2> rank4;
    rank4.fill(1.0);
    std::cout << "   Created 2x2x2x2 tensor (rank-4)" << std::endl;
    std::cout << "   size = " << rank4.size() << " elements" << std::endl;
    std::cout << "   rank = " << Tensor<double, 2, 2, 2, 2>::rank << std::endl << std::endl;

    // 7. Common use cases
    std::cout << "7. Common Use Cases:" << std::endl;

    // 3D voxel grid
    Tensor<float, 8, 8, 8> voxels;
    voxels.fill(0.0f);
    voxels(4, 4, 4) = 1.0f; // Set center voxel
    std::cout << "   8x8x8 voxel grid, center = " << voxels(4, 4, 4) << std::endl;

    // RGB image (small)
    Tensor<uint8_t, 4, 4, 3> image; // height x width x channels
    image.fill(128);
    std::cout << "   4x4 RGB image, pixel(0,0,0) = " << static_cast<int>(image(0, 0, 0)) << std::endl;

    // Batch of feature maps (batch x channels x height x width)
    Tensor<float, 2, 3, 4, 4> batch;
    batch.fill(0.5f);
    std::cout << "   Batch of 2x3x4x4 feature maps" << std::endl;

    return 0;
}
