#include <datapod/matrix/tensor.hpp>
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== tensor Usage Examples ===" << std::endl << std::endl;

    // 1. Construction
    std::cout << "1. Construction:" << std::endl;
    tensor<double, 3> position{1.0, 2.0, 3.0};  // 3D position vector
    tensor<float, 6> state;                     // 6-DOF state vector
    auto features = tensor{0.5, 0.8, 0.3, 0.9}; // Deduction guide

    std::cout << "   position[0] = " << position[0] << std::endl;
    std::cout << "   position size = " << position.size() << std::endl;
    std::cout << "   features deduced size = " << features.size() << std::endl << std::endl;

    // 2. Element access
    std::cout << "2. Element Access:" << std::endl;
    tensor<double, 4> vec{10.0, 20.0, 30.0, 40.0};
    std::cout << "   vec[0] = " << vec[0] << std::endl;
    std::cout << "   vec.front() = " << vec.front() << std::endl;
    std::cout << "   vec.back() = " << vec.back() << std::endl;
    std::cout << "   vec.at(2) = " << vec.at(2) << std::endl << std::endl;

    // 3. Operations
    std::cout << "3. Operations:" << std::endl;
    tensor<double, 5> t;
    t.fill(7.5);
    std::cout << "   After fill(7.5): t[0] = " << t[0] << ", t[4] = " << t[4] << std::endl;

    tensor<int, 3> a{1, 2, 3};
    tensor<int, 3> b{10, 20, 30};
    a.swap(b);
    std::cout << "   After swap: a[0] = " << a[0] << ", b[0] = " << b[0] << std::endl << std::endl;

    // 4. Iteration
    std::cout << "4. Iteration:" << std::endl;
    tensor<int, 5> nums{1, 2, 3, 4, 5};
    std::cout << "   Elements: ";
    for (auto val : nums) {
        std::cout << val << " ";
    }
    std::cout << std::endl << std::endl;

    // 5. Type traits
    std::cout << "5. Type Traits:" << std::endl;
    std::cout << "   is_tensor_v<tensor<double,3>>: " << is_tensor_v<tensor<double, 3>> << std::endl;
    std::cout << "   rank: " << tensor<double, 3>::rank << " (rank-1 tensor)" << std::endl;
    std::cout << "   size: " << tensor<double, 3>::size_ << std::endl << std::endl;

    // 6. Use cases
    std::cout << "6. Common Use Cases:" << std::endl;

    // 3D position
    tensor3d pos3d{1.0, 2.0, 3.0};
    std::cout << "   3D position: (" << pos3d[0] << ", " << pos3d[1] << ", " << pos3d[2] << ")" << std::endl;

    // 6-DOF state (position + velocity)
    tensor6d robot_state;
    robot_state.fill(0.0);
    robot_state[0] = 1.0; // x
    robot_state[1] = 2.0; // y
    robot_state[2] = 3.0; // z
    std::cout << "   Robot state: position = (" << robot_state[0] << ", " << robot_state[1] << ", " << robot_state[2]
              << ")" << std::endl;

    return 0;
}
