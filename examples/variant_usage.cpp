#include <datapod/adapters/variant.hpp>
#include <iostream>
#include <string>

using namespace datapod;

void example_basic_usage() {
    std::cout << "=== Basic Usage ===" << std::endl;

    // Variant can hold one of several types
    Variant<int, double, std::string> v;

    std::cout << "Default constructed - valid: " << (v.valid() ? "yes" : "no") << std::endl;

    // Assign an int
    v = 42;
    std::cout << "After assigning 42 - index: " << v.index() << std::endl;
    std::cout << "Value: " << v.as<int>() << std::endl;

    // Assign a double
    v = 3.14;
    std::cout << "After assigning 3.14 - index: " << v.index() << std::endl;
    std::cout << "Value: " << v.as<double>() << std::endl;

    // Assign a string
    v = std::string("Hello Variant!");
    std::cout << "After assigning string - index: " << v.index() << std::endl;
    std::cout << "Value: " << v.as<std::string>() << std::endl;

    std::cout << std::endl;
}

void example_construction() {
    std::cout << "=== Construction ===" << std::endl;

    // Construct with value
    Variant<int, double, std::string> v1(42);
    std::cout << "v1 (int): " << v1.as<int>() << std::endl;

    Variant<int, double, std::string> v2(3.14);
    std::cout << "v2 (double): " << v2.as<double>() << std::endl;

    Variant<int, double, std::string> v3(std::string("constructed"));
    std::cout << "v3 (string): " << v3.as<std::string>() << std::endl;

    // Copy construction
    Variant<int, double, std::string> v4(v1);
    std::cout << "v4 (copy of v1): " << v4.as<int>() << std::endl;

    // Move construction
    Variant<int, double, std::string> v5(std::move(v3));
    std::cout << "v5 (moved from v3): " << v5.as<std::string>() << std::endl;

    std::cout << std::endl;
}

void example_as_method() {
    std::cout << "=== As Method (Type Access) ===" << std::endl;

    Variant<int, double, std::string> v(42);

    // Access value with as<T>()
    std::cout << "Value as int: " << v.as<int>() << std::endl;

    // Modify through as<T>()
    v.as<int>() = 100;
    std::cout << "After modification: " << v.as<int>() << std::endl;

    // Change type
    v = std::string("mutable");
    v.as<std::string>() += " string";
    std::cout << "Modified string: " << v.as<std::string>() << std::endl;

    std::cout << std::endl;
}

void example_get_functions() {
    std::cout << "=== Get Functions ===" << std::endl;

    Variant<int, double, std::string> v(42);

    // get<I> - by index
    std::cout << "get<0>: " << get<0>(v) << std::endl;

    // get<T> - by type
    std::cout << "get<int>: " << get<int>(v) << std::endl;

    // get_if - returns pointer (nullptr if wrong type)
    if (auto *p = get_if<int>(v)) {
        std::cout << "get_if<int>: " << *p << std::endl;
        *p = 200; // Can modify through pointer
        std::cout << "After modification: " << v.as<int>() << std::endl;
    }

    if (auto *p = get_if<double>(v)) {
        std::cout << "get_if<double>: " << *p << std::endl;
    } else {
        std::cout << "get_if<double>: nullptr (wrong type)" << std::endl;
    }

    std::cout << std::endl;
}

void example_emplace() {
    std::cout << "=== Emplace ===" << std::endl;

    Variant<int, double, std::string> v;

    // Emplace by type
    v.emplace<int>(42);
    std::cout << "After emplace<int>(42): " << v.as<int>() << std::endl;

    // Emplace by index
    v.emplace<2>("emplaced string");
    std::cout << "After emplace<2>: " << v.as<std::string>() << std::endl;

    // Emplace with constructor arguments
    v.emplace<std::string>("hello");
    std::cout << "After emplace<std::string>: " << v.as<std::string>() << std::endl;

    std::cout << std::endl;
}

void example_apply() {
    std::cout << "=== Apply (Visitor Pattern) ===" << std::endl;

    Variant<int, double, std::string> v(42);

    // Apply a lambda
    v.apply([](auto &&val) {
        using T = std::decay_t<decltype(val)>;
        std::cout << "Visiting value: ";
        if constexpr (std::is_same_v<T, int>) {
            std::cout << "int " << val << std::endl;
        } else if constexpr (std::is_same_v<T, double>) {
            std::cout << "double " << val << std::endl;
        } else if constexpr (std::is_same_v<T, std::string>) {
            std::cout << "string " << val << std::endl;
        }
    });

    // Apply that returns a value
    auto result = v.apply([](auto &&val) -> int {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, int>) {
            return val * 2;
        } else if constexpr (std::is_same_v<T, double>) {
            return static_cast<int>(val * 2);
        } else {
            return 0;
        }
    });
    std::cout << "Result of transformation: " << result << std::endl;

    // Apply that modifies
    v.apply([](auto &&val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, int>) {
            val = 100;
        }
    });
    std::cout << "After modification: " << v.as<int>() << std::endl;

    std::cout << std::endl;
}

void example_std_visit() {
    std::cout << "=== std::visit ===" << std::endl;

    Variant<int, double, std::string> v(3.14);

    // std::visit works with datapod::Variant
    auto result = std::visit(
        [](auto &&val) -> std::string {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, int>) {
                return "Type: int, Value: " + std::to_string(val);
            } else if constexpr (std::is_same_v<T, double>) {
                return "Type: double, Value: " + std::to_string(val);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return "Type: string, Value: " + val;
            }
            return "Unknown";
        },
        v);

    std::cout << result << std::endl;

    std::cout << std::endl;
}

void example_comparisons() {
    std::cout << "=== Comparisons ===" << std::endl;

    Variant<int, double> v1(42);
    Variant<int, double> v2(42);
    Variant<int, double> v3(100);
    Variant<int, double> v4(3.14);

    std::cout << "v1 == v2: " << (v1 == v2 ? "true" : "false") << std::endl;
    std::cout << "v1 != v3: " << (v1 != v3 ? "true" : "false") << std::endl;
    std::cout << "v1 < v3: " << (v1 < v3 ? "true" : "false") << std::endl;

    // Different types compare by index
    std::cout << "v1 < v4 (int vs double): " << (v1 < v4 ? "true" : "false") << std::endl;

    std::cout << std::endl;
}

void example_swap() {
    std::cout << "=== Swap ===" << std::endl;

    Variant<int, double, std::string> v1(42);
    Variant<int, double, std::string> v2(std::string("world"));

    std::cout << "Before swap:" << std::endl;
    std::cout << "  v1 index: " << v1.index() << ", value: " << v1.as<int>() << std::endl;
    std::cout << "  v2 index: " << v2.index() << ", value: " << v2.as<std::string>() << std::endl;

    v1.swap(v2);

    std::cout << "After swap:" << std::endl;
    std::cout << "  v1 index: " << v1.index() << ", value: " << v1.as<std::string>() << std::endl;
    std::cout << "  v2 index: " << v2.index() << ", value: " << v2.as<int>() << std::endl;

    std::cout << std::endl;
}

void example_index_and_valid() {
    std::cout << "=== Index and Valid ===" << std::endl;

    Variant<int, double, std::string> v;

    std::cout << "Default constructed:" << std::endl;
    std::cout << "  valid: " << (v.valid() ? "true" : "false") << std::endl;
    std::cout << "  operator bool: " << (v ? "true" : "false") << std::endl;

    v = 42;
    std::cout << "After assigning int:" << std::endl;
    std::cout << "  valid: " << (v.valid() ? "true" : "false") << std::endl;
    std::cout << "  index: " << v.index() << " (int is at index 0)" << std::endl;

    v = 3.14;
    std::cout << "After assigning double:" << std::endl;
    std::cout << "  index: " << v.index() << " (double is at index 1)" << std::endl;

    v = std::string("test");
    std::cout << "After assigning string:" << std::endl;
    std::cout << "  index: " << v.index() << " (string is at index 2)" << std::endl;

    std::cout << std::endl;
}

void example_variant_size() {
    std::cout << "=== Variant Size ===" << std::endl;

    std::cout << "variant_size_v<Variant<int>>: " << variant_size_v<Variant<int>> << std::endl;
    std::cout << "variant_size_v<Variant<int, double>>: " << variant_size_v<Variant<int, double>> << std::endl;
    std::cout << "variant_size_v<Variant<int, double, std::string>>: "
              << variant_size_v<Variant<int, double, std::string>> << std::endl;

    std::cout << std::endl;
}

void example_custom_type() {
    std::cout << "=== Custom Type ===" << std::endl;

    struct Point {
        int x, y;
        Point(int x_, int y_) : x(x_), y(y_) {}
    };

    Variant<int, Point, std::string> v;

    v.emplace<Point>(10, 20);
    std::cout << "Point: (" << v.as<Point>().x << ", " << v.as<Point>().y << ")" << std::endl;

    // Modify
    v.as<Point>().x = 30;
    std::cout << "After modification: (" << v.as<Point>().x << ", " << v.as<Point>().y << ")" << std::endl;

    std::cout << std::endl;
}

void example_use_case_result_type() {
    std::cout << "=== Use Case: Result Type ===" << std::endl;

    // Variant can be used as a result type (success or error)
    using Result = Variant<int, std::string>; // int = success, string = error

    auto divide = [](int a, int b) -> Result {
        if (b == 0) {
            return std::string("Error: Division by zero");
        }
        return a / b;
    };

    auto result1 = divide(10, 2);
    result1.apply([](auto &&val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, int>) {
            std::cout << "Success: " << val << std::endl;
        } else {
            std::cout << "Error: " << val << std::endl;
        }
    });

    auto result2 = divide(10, 0);
    result2.apply([](auto &&val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, int>) {
            std::cout << "Success: " << val << std::endl;
        } else {
            std::cout << "Error: " << val << std::endl;
        }
    });

    std::cout << std::endl;
}

void example_use_case_state_machine() {
    std::cout << "=== Use Case: State Machine ===" << std::endl;

    struct Idle {
        std::string name = "Idle";
    };
    struct Running {
        int speed = 0;
    };
    struct Stopped {
        std::string reason;
    };

    using State = Variant<Idle, Running, Stopped>;

    State state;
    state.emplace<Idle>();

    auto print_state = [](State const &s) {
        s.apply([](auto &&val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, Idle>) {
                std::cout << "State: " << val.name << std::endl;
            } else if constexpr (std::is_same_v<T, Running>) {
                std::cout << "State: Running at speed " << val.speed << std::endl;
            } else if constexpr (std::is_same_v<T, Stopped>) {
                std::cout << "State: Stopped - " << val.reason << std::endl;
            }
        });
    };

    print_state(state);

    // Transition to Running
    state.emplace<Running>();
    state.as<Running>().speed = 60;
    print_state(state);

    // Transition to Stopped
    state.emplace<Stopped>();
    state.as<Stopped>().reason = "User requested";
    print_state(state);

    std::cout << std::endl;
}

void example_type_checking() {
    std::cout << "=== Type Checking ===" << std::endl;

    Variant<int, double, std::string> v(42);

    // Check type using get_if
    if (get_if<int>(v)) {
        std::cout << "Variant holds an int" << std::endl;
    }

    if (!get_if<double>(v)) {
        std::cout << "Variant does NOT hold a double" << std::endl;
    }

    // Check using index
    if (v.index() == 0) {
        std::cout << "Index is 0 (int)" << std::endl;
    }

    std::cout << std::endl;
}

int main() {
    std::cout << "DataPod Variant Usage Examples" << std::endl;
    std::cout << "===============================" << std::endl << std::endl;

    example_basic_usage();
    example_construction();
    example_as_method();
    example_get_functions();
    example_emplace();
    example_apply();
    example_std_visit();
    example_comparisons();
    example_swap();
    example_index_and_valid();
    example_variant_size();
    example_custom_type();
    example_use_case_result_type();
    example_use_case_state_machine();
    example_type_checking();

    std::cout << "All examples completed successfully!" << std::endl;

    return 0;
}
