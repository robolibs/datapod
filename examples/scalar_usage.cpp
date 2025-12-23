#include <datapod/matrix/scalar.hpp>
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== scalar Usage Examples ===" << std::endl << std::endl;

    // 1. Construction
    std::cout << "1. Construction:" << std::endl;
    scalar<double> temperature{23.5};
    scalar<float> mass{10.5f};
    scalar<int> count{42};

    std::cout << "   temperature = " << temperature.value << std::endl;
    std::cout << "   mass = " << mass.value << std::endl;
    std::cout << "   count = " << count.value << std::endl << std::endl;

    // 2. Implicit conversion
    std::cout << "2. Implicit Conversion:" << std::endl;
    scalar<double> s{100.0};
    double raw_value = s; // Implicit conversion to underlying type
    std::cout << "   scalar<double> → double: " << raw_value << std::endl << std::endl;

    // 3. Arithmetic operations
    std::cout << "3. Arithmetic Operations:" << std::endl;
    scalar<double> a{10.0};
    scalar<double> b{3.0};

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
    scalar<double> x{50.0};
    x += 10.0; // Works with raw values too
    std::cout << "   x after += 10.0: " << x.value << std::endl;
    x -= 5.0;
    std::cout << "   x after -= 5.0: " << x.value << std::endl << std::endl;

    // 5. Comparison
    std::cout << "5. Comparison:" << std::endl;
    scalar<int> p{10};
    scalar<int> q{20};
    std::cout << "   scalar(10) < scalar(20): " << (p < q ? "true" : "false") << std::endl;
    std::cout << "   scalar(10) == 10: " << (p == 10 ? "true" : "false") << std::endl << std::endl;

    // 6. Type traits
    std::cout << "6. Type Traits:" << std::endl;
    std::cout << "   is_scalar_v<scalar<double>>: " << is_scalar_v<scalar<double>> << std::endl;
    std::cout << "   rank: " << scalar<double>::rank << " (rank-0 tensor)" << std::endl;

    return 0;
}
