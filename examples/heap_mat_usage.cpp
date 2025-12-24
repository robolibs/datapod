/**
 * @file heap_mat_usage.cpp
 * @brief Demonstrates heap-allocated mat::vector, mat::matrix, and mat::heap_tensor
 *
 * This example shows how datapod automatically uses heap allocation for large
 * mathematical types that would overflow the stack, while keeping small types
 * stack-allocated for optimal performance.
 *
 * Key concepts:
 * - HEAP_THRESHOLD (1024 elements): Types with more elements use heap
 * - Stack types: POD, trivially copyable, zero-copy serialization
 * - Heap types: SIMD-aligned, proper copy/move semantics, explicit serialization
 * - Same API for both stack and heap versions (transparent to users)
 */

#include <datapod/matrix/matrix.hpp>
#include <datapod/matrix/tensor.hpp>
#include <datapod/matrix/vector.hpp>
#include <datapod/serialization/serialize.hpp>
#include <iomanip>
#include <iostream>

using namespace datapod;
using namespace datapod::mat;

int main() {
    std::cout << "=== Heap-Allocated Mat Types Usage Examples ===" << std::endl << std::endl;

    // =========================================================================
    // 1. HEAP THRESHOLD DEMONSTRATION
    // =========================================================================
    std::cout << "1. Heap Threshold (HEAP_THRESHOLD = " << HEAP_THRESHOLD << " elements):" << std::endl;
    std::cout << std::endl;

    // Small vector - stack allocated
    std::cout << "   vector<double, 3>:" << std::endl;
    std::cout << "      uses_heap = " << std::boolalpha << vector<double, 3>::uses_heap << std::endl;
    std::cout << "      is_pod    = " << vector<double, 3>::is_pod << std::endl;

    // Large vector - heap allocated
    std::cout << "   vector<double, 2000>:" << std::endl;
    std::cout << "      uses_heap = " << vector<double, 2000>::uses_heap << std::endl;
    std::cout << "      is_pod    = " << vector<double, 2000>::is_pod << std::endl;

    // Small matrix - stack allocated
    std::cout << "   matrix<double, 3, 3> (9 elements):" << std::endl;
    std::cout << "      uses_heap = " << matrix<double, 3, 3>::uses_heap << std::endl;
    std::cout << "      is_pod    = " << matrix<double, 3, 3>::is_pod << std::endl;

    // Large matrix - heap allocated
    std::cout << "   matrix<double, 50, 50> (2500 elements):" << std::endl;
    std::cout << "      uses_heap = " << matrix<double, 50, 50>::uses_heap << std::endl;
    std::cout << "      is_pod    = " << matrix<double, 50, 50>::is_pod << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // 2. LARGE VECTOR USAGE (heap-allocated)
    // =========================================================================
    std::cout << "2. Large Vector (ML embeddings, 10000 elements):" << std::endl;

    // This would crash with stack allocation (40KB on stack!)
    // With heap allocation, it works fine
    vector<float, 10000> embeddings;
    embeddings.fill(0.0f);

    // Set some values
    embeddings[0] = 1.0f;
    embeddings[5000] = 0.5f;
    embeddings[9999] = -1.0f;

    std::cout << "   embeddings[0]    = " << embeddings[0] << std::endl;
    std::cout << "   embeddings[5000] = " << embeddings[5000] << std::endl;
    std::cout << "   embeddings[9999] = " << embeddings[9999] << std::endl;
    std::cout << "   size             = " << embeddings.size() << " elements" << std::endl;
    std::cout << "   memory           = " << (embeddings.size() * sizeof(float)) / 1024 << " KB" << std::endl;

    // SIMD alignment check
    bool aligned = (reinterpret_cast<uintptr_t>(embeddings.data()) % 32 == 0);
    std::cout << "   32-byte aligned  = " << std::boolalpha << aligned << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // 3. LARGE MATRIX USAGE (heap-allocated)
    // =========================================================================
    std::cout << "3. Large Matrix (image data, 100x100):" << std::endl;

    // 100x100 matrix = 10000 elements = 80KB for double
    matrix<double, 100, 100> image;
    image.fill(0.0);

    // Set some pixel values
    image(0, 0) = 255.0;   // Top-left
    image(50, 50) = 128.0; // Center
    image(99, 99) = 64.0;  // Bottom-right

    std::cout << "   image(0,0)   = " << image(0, 0) << std::endl;
    std::cout << "   image(50,50) = " << image(50, 50) << std::endl;
    std::cout << "   image(99,99) = " << image(99, 99) << std::endl;
    std::cout << "   rows x cols  = " << image.rows() << " x " << image.cols() << std::endl;
    std::cout << "   memory       = " << (image.size() * sizeof(double)) / 1024 << " KB" << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // 4. HEAP TENSOR USAGE
    // =========================================================================
    std::cout << "4. Heap Tensor (3D volume, 20x20x20):" << std::endl;

    // heap_tensor is explicitly for large tensors
    heap_tensor<float, 20, 20, 20> volume; // 8000 elements
    volume.fill(0.0f);

    // Set some voxel values
    volume(0, 0, 0) = 1.0f;
    volume(10, 10, 10) = 0.5f;
    volume(19, 19, 19) = 0.25f;

    std::cout << "   volume(0,0,0)     = " << volume(0, 0, 0) << std::endl;
    std::cout << "   volume(10,10,10)  = " << volume(10, 10, 10) << std::endl;
    std::cout << "   volume(19,19,19)  = " << volume(19, 19, 19) << std::endl;
    std::cout << "   shape             = " << volume.dim(0) << "x" << volume.dim(1) << "x" << volume.dim(2)
              << std::endl;
    std::cout << "   total elements    = " << volume.size() << std::endl;
    std::cout << "   memory            = " << (volume.size() * sizeof(float)) / 1024 << " KB" << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // 5. COPY AND MOVE SEMANTICS
    // =========================================================================
    std::cout << "5. Copy and Move Semantics:" << std::endl;

    vector<double, 2000> v1;
    v1.fill(42.0);

    // Copy - creates independent copy
    vector<double, 2000> v2 = v1;
    v1[0] = 999.0;
    std::cout << "   After copy, v1[0] = " << v1[0] << ", v2[0] = " << v2[0] << std::endl;

    // Move - transfers ownership (efficient)
    vector<double, 2000> v3 = std::move(v1);
    std::cout << "   After move, v3[0] = " << v3[0] << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // 6. SERIALIZATION
    // =========================================================================
    std::cout << "6. Serialization (round-trip):" << std::endl;

    vector<float, 2000> original;
    for (size_t i = 0; i < 2000; ++i) {
        original[i] = static_cast<float>(i) * 0.001f;
    }

    // Serialize to byte buffer
    auto buf = serialize(original);
    std::cout << "   Serialized size = " << buf.size() << " bytes" << std::endl;

    // Deserialize back
    auto restored = deserialize<Mode::NONE, vector<float, 2000>>(buf);

    // Verify
    bool match = true;
    for (size_t i = 0; i < 2000; ++i) {
        if (restored[i] != original[i]) {
            match = false;
            break;
        }
    }
    std::cout << "   Round-trip match = " << std::boolalpha << match << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // 7. COMPARISON: STACK VS HEAP TRANSPARENT API
    // =========================================================================
    std::cout << "7. Transparent API (same code for stack/heap):" << std::endl;

    // Small vector (stack)
    vector<double, 3> small_vec{1.0, 2.0, 3.0};

    // Large vector (heap)
    vector<double, 2000> large_vec;
    large_vec[0] = 1.0;
    large_vec[1] = 2.0;
    large_vec[2] = 3.0;

    // Same API works for both!
    std::cout << "   small_vec[0] = " << small_vec[0] << " (stack)" << std::endl;
    std::cout << "   large_vec[0] = " << large_vec[0] << " (heap)" << std::endl;
    std::cout << "   small_vec.size() = " << small_vec.size() << std::endl;
    std::cout << "   large_vec.size() = " << large_vec.size() << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // 8. USE CASES SUMMARY
    // =========================================================================
    std::cout << "8. Use Cases Summary:" << std::endl;
    std::cout << std::endl;
    std::cout << "   STACK (N <= 1024, POD, zero-copy):" << std::endl;
    std::cout << "      - Robotics: vector<double, 3> for position" << std::endl;
    std::cout << "      - Robotics: matrix<double, 3, 3> for rotation" << std::endl;
    std::cout << "      - Robotics: matrix<double, 6, 6> for covariance" << std::endl;
    std::cout << std::endl;
    std::cout << "   HEAP (N > 1024, SIMD-aligned):" << std::endl;
    std::cout << "      - ML: vector<float, 1000000> for embeddings" << std::endl;
    std::cout << "      - Image: matrix<float, 1024, 1024> for pixels" << std::endl;
    std::cout << "      - Volume: heap_tensor<float, 256, 256, 256> for 3D data" << std::endl;
    std::cout << std::endl;

    std::cout << "=== Done ===" << std::endl;
    return 0;
}
