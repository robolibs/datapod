#include "datapod/datapod.hpp"
#include <iostream>

using namespace datapod;

// Helper function that might fail
Optional<int> parse_int(String const &str) {
    if (str.view() == "42") {
        return Optional<int>(42);
    }
    return Optional<int>{}; // Empty optional on failure
}

int main() {
    std::cout << "=== Optional Usage Examples ===" << std::endl << std::endl;

    // Example 1: Basic usage
    {
        std::cout << "1. Basic Optional usage:" << std::endl;

        Optional<int> maybe_value(10);
        Optional<int> no_value;

        std::cout << "   has_value: " << maybe_value.has_value() << std::endl;
        std::cout << "   value: " << *maybe_value << std::endl;
        std::cout << "   empty has_value: " << no_value.has_value() << std::endl;
        std::cout << std::endl;
    }

    // Example 2: value_or()
    {
        std::cout << "2. value_or() - Provide default value:" << std::endl;

        Optional<int> maybe(5);
        Optional<int> empty;

        std::cout << "   With value: " << maybe.value_or(100) << std::endl;
        std::cout << "   Without value: " << empty.value_or(100) << std::endl;
        std::cout << std::endl;
    }

    // Example 3: transform() - Map operation
    {
        std::cout << "3. transform() - Transform value if present:" << std::endl;

        Optional<int> opt(10);
        auto doubled = opt.transform([](int x) { return x * 2; });
        auto as_string = doubled.transform([](int x) { return String::format("Value: {}", x); });

        if (as_string.has_value()) {
            std::cout << "   Result: " << as_string->view() << std::endl;
        }
        std::cout << std::endl;
    }

    // Example 4: and_then() - Flatmap operation
    {
        std::cout << "4. and_then() - Chain operations that return Optional:" << std::endl;

        auto result = parse_int(String("42"))
                          .and_then([](int x) { return x > 0 ? Optional<int>(x * 2) : Optional<int>{}; })
                          .transform([](int x) { return String::format("Result: {}", x); });

        if (result.has_value()) {
            std::cout << "   " << result->view() << std::endl;
        }
        std::cout << std::endl;
    }

    // Example 5: or_else() - Provide alternative
    {
        std::cout << "5. or_else() - Provide fallback Optional:" << std::endl;

        Optional<int> empty;
        auto with_fallback = empty.or_else([]() { return Optional<int>(999); });

        std::cout << "   Fallback value: " << *with_fallback << std::endl;
        std::cout << std::endl;
    }

    // Example 6: Serialization
    {
        std::cout << "6. Serialization:" << std::endl;

        Optional<int> original(42);
        std::cout << "   Original has value: " << original.has_value() << std::endl;

        // Serialize
        auto buffer = serialize(original);
        std::cout << "   Serialized to " << buffer.size() << " bytes" << std::endl;

        // Deserialize
        auto restored = deserialize<Mode::NONE, Optional<int>>(buffer);
        std::cout << "   Restored has value: " << restored.has_value() << std::endl;
        std::cout << "   Restored value: " << *restored << std::endl;
        std::cout << std::endl;
    }

    // Example 7: Complex pipeline (C++23 monadic style)
    {
        std::cout << "7. Complex pipeline (monadic composition):" << std::endl;

        // Simulate: parse string -> validate -> double -> format
        auto result = parse_int(String("42"))
                          .and_then([](int x) {
                              // Validate: only accept values < 100
                              return x < 100 ? Optional<int>(x) : Optional<int>{};
                          })
                          .transform([](int x) {
                              // Double the value
                              return x * 2;
                          })
                          .transform([](int x) {
                              // Format as string
                              return String::format("Final: {}", x);
                          })
                          .or_else([]() {
                              // Fallback if any step failed
                              return Optional<String>(String("Processing failed"));
                          });

        std::cout << "   " << result->view() << std::endl;
        std::cout << std::endl;
    }

    std::cout << "=== Optional Examples Complete ===" << std::endl;
    return 0;
}
