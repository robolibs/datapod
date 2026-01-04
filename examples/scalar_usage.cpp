#include <datapod/pods/matrix/scalar.hpp>
#include <iostream>

using namespace datapod::mat;

int main() {
    std::cout << "=== Scalar Usage Examples ===" << std::endl << std::endl;

    // 1. Construction
    std::cout << "1. Construction:" << std::endl;
    Scalar<double> temperature{23.5};
    Scalar<float> mass{10.5f};
    Scalar<int> count{42};

    std::cout << "   temperature = " << temperature.value << std::endl;
    std::cout << "   mass = " << mass.value << std::endl;
    std::cout << "   count = " << count.value << std::endl << std::endl;

    // 2. Implicit conversion
    std::cout << "2. Implicit Conversion:" << std::endl;
    Scalar<double> s{100.0};
    double raw_value = s; // Implicit conversion to underlying type
    std::cout << "   Scalar<double> → double: " << raw_value << std::endl << std::endl;

    // 3. Arithmetic operations
    std::cout << "3. Arithmetic Operations:" << std::endl;
    Scalar<double> a{10.0};
    Scalar<double> b{3.0};

    auto sum = a + b;
    auto diff = a - b;
    auto prod = a * b;
    auto quot = a / b;

    std::cout << "   10.0 + 3.0 = " << sum.value << std::endl;
    std::cout << "   10.0 - 3.0 = " << diff.value << std::endl;
    std::cout << "   10.0 * 3.0 = " << prod.value << std::endl;
    std::cout << "   10.0 / 3.0 = " << quot.value << std::endl << std::endl;

    // 4. Compound assignment
    std::cout << "4. Compound Assignment:" << std::endl;
    Scalar<double> x{50.0};
    x += 10.0; // Works with raw values too
    std::cout << "   x after += 10.0: " << x.value << std::endl;
    x -= 5.0;
    std::cout << "   x after -= 5.0: " << x.value << std::endl << std::endl;

    // 5. Comparison
    std::cout << "5. Comparison:" << std::endl;
    Scalar<int> p{10};
    Scalar<int> q{20};
    std::cout << "   Scalar(10) < Scalar(20): " << (p < q ? "true" : "false") << std::endl;
    std::cout << "   Scalar(10) == 10: " << (p == 10 ? "true" : "false") << std::endl << std::endl;

    // 6. Type traits
    std::cout << "6. Type Traits:" << std::endl;
    std::cout << "   is_scalar_v<Scalar<double>>: " << is_scalar_v<Scalar<double>> << std::endl;
    std::cout << "   rank: " << Scalar<double>::rank << " (rank-0 tensor)" << std::endl;

    return 0;
}
