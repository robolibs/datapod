#include <datapod/pods/adapters/error.hpp>
#include <datapod/pods/adapters/result.hpp>
#include <datapod/core/none.hpp>
#include <iostream>

using namespace datapod;

// Example 1: Safe division
Result<int, Error> safe_divide(int a, int b) {
    if (b == 0) {
        return Result<int, Error>::err(Error::invalid_argument("Division by zero"));
    }
    return Result<int, Error>::ok(a / b);
}

// Example 2: File reading simulation
Result<String, Error> read_file(const String &path) {
    if (path.empty()) {
        return Result<String, Error>::err(Error::invalid_argument("Empty file path"));
    }
    if (path == "nonexistent.txt") {
        return Result<String, Error>::err(Error::not_found("File does not exist"));
    }
    if (path == "forbidden.txt") {
        return Result<String, Error>::err(Error::permission_denied("Access denied"));
    }
    return Result<String, Error>::ok("File contents: Hello, World!");
}

// Example 3: Chaining operations
Result<int, Error> parse_and_double(const String &input) {
    // Simulate parsing
    if (input.empty()) {
        return Result<int, Error>::err(Error::parse_error("Empty string"));
    }
    if (input == "NaN") {
        return Result<int, Error>::err(Error::parse_error("Not a number"));
    }

    // Simulate successful parse (just return 42 for demo)
    int value = 42;

    // Chain: parse -> double -> validate range
    return Result<int, Error>::ok(value).map([](int x) { return x * 2; }).and_then([](int x) -> Result<int, Error> {
        if (x > 100) {
            return Result<int, Error>::err(Error::out_of_range("Value too large"));
        }
        return Result<int, Error>::ok(x);
    });
}

// Example 4: Using None with pointers
struct Config {
    String name;
    int value;
};

const Config *find_config(const String &key) {
    static Config cfg{"default", 100};
    if (key == "default") {
        return &cfg;
    }
    return None; // Using None instead of nullptr
}

int main() {
    std::cout << "=== Result<T, E> and Error Usage Examples ===" << std::endl;

    // Example 1: Safe division
    std::cout << "\n--- Safe Division ---" << std::endl;
    auto result1 = safe_divide(10, 2);
    if (result1.is_ok()) {
        std::cout << "10 / 2 = " << result1.value() << std::endl;
    }

    auto result2 = safe_divide(10, 0);
    if (result2.is_err()) {
        std::cout << "Error: " << result2.error().message.c_str() << std::endl;
        std::cout << "Error code: " << result2.error().code << std::endl;
    }

    // Using value_or
    std::cout << "Result with default: " << result2.value_or(-1) << std::endl;

    // Example 2: File reading
    std::cout << "\n--- File Reading ---" << std::endl;
    auto file1 = read_file("data.txt");
    if (file1.is_ok()) {
        std::cout << file1.value().c_str() << std::endl;
    }

    auto file2 = read_file("nonexistent.txt");
    if (file2.is_err()) {
        std::cout << "Error: " << file2.error().message.c_str() << " (code: " << file2.error().code << ")" << std::endl;
    }

    auto file3 = read_file("");
    if (file3.is_err()) {
        std::cout << "Error: " << file3.error().message.c_str() << std::endl;
    }

    // Example 3: Chaining operations
    std::cout << "\n--- Chaining Operations ---" << std::endl;
    auto chain1 = parse_and_double("42");
    if (chain1.is_ok()) {
        std::cout << "Parsed and doubled: " << chain1.value() << std::endl;
    }

    auto chain2 = parse_and_double("");
    if (chain2.is_err()) {
        std::cout << "Chain error: " << chain2.error().message.c_str() << std::endl;
    }

    // Example 4: Using Result with map
    std::cout << "\n--- Map Transformation ---" << std::endl;
    auto mapped = Result<int, Error>::ok(5).map([](int x) { return x * x; }).map([](int x) { return x + 10; });

    if (mapped.is_ok()) {
        std::cout << "Mapped result: " << mapped.value() << std::endl; // 5^2 + 10 = 35
    }

    // Example 5: Error recovery with or_else
    std::cout << "\n--- Error Recovery ---" << std::endl;
    auto recovered = Result<int, Error>::err(Error::timeout("Operation timed out"))
                         .or_else([](const Error &e) -> Result<int, Error> {
                             std::cout << "Recovering from error: " << e.message.c_str() << std::endl;
                             return Result<int, Error>::ok(-1); // Return default value
                         });

    std::cout << "Recovered value: " << recovered.value() << std::endl;

    // Example 6: Using None with pointers
    std::cout << "\n--- Using None ---" << std::endl;
    const Config *cfg1 = find_config("default");
    if (cfg1 != None) {
        std::cout << "Found config: " << cfg1->name.c_str() << " = " << cfg1->value << std::endl;
    }

    const Config *cfg2 = find_config("unknown");
    if (cfg2 == None) {
        std::cout << "Config not found (using None)" << std::endl;
    }

    // Example 7: Common error types
    std::cout << "\n--- Common Error Types ---" << std::endl;
    Error err1 = Error::not_found("Resource missing");
    Error err2 = Error::timeout("Request timed out");
    Error err3 = Error::permission_denied("Insufficient privileges");

    std::cout << "NOT_FOUND: " << err1.message.c_str() << std::endl;
    std::cout << "TIMEOUT: " << err2.message.c_str() << std::endl;
    std::cout << "PERMISSION_DENIED: " << err3.message.c_str() << std::endl;

    // Example 8: Pattern matching style
    std::cout << "\n--- Pattern Matching Style ---" << std::endl;
    auto process = [](Result<int, Error> result) {
        if (result.is_ok()) {
            std::cout << "Success: " << result.value() << std::endl;
        } else {
            switch (result.error().code) {
            case Error::INVALID_ARGUMENT:
                std::cout << "Invalid input!" << std::endl;
                break;
            case Error::OUT_OF_RANGE:
                std::cout << "Value out of range!" << std::endl;
                break;
            default:
                std::cout << "Other error: " << result.error().message.c_str() << std::endl;
            }
        }
    };

    process(Result<int, Error>::ok(42));
    process(Result<int, Error>::err(Error::invalid_argument("Bad input")));
    process(Result<int, Error>::err(Error::out_of_range("Too big")));

    return 0;
}
