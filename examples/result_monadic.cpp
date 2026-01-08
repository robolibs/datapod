#include <datapod/pods/adapters/error.hpp>
#include <datapod/pods/adapters/result.hpp>
#include <iostream>

using namespace datapod;

// Helper functions for examples
Result<int, Error> divide(int a, int b) {
    if (b == 0) {
        return Result<int, Error>::err(Error::invalid_argument("Division by zero"));
    }
    return Result<int, Error>::ok(a / b);
}

Result<int, Error> parse_int(const String &s) {
    if (s.empty()) {
        return Result<int, Error>::err(Error::parse_error("Empty string"));
    }
    if (s == "NaN") {
        return Result<int, Error>::err(Error::parse_error("Not a number"));
    }
    // Simplified: just return 42 for demo
    return Result<int, Error>::ok(42);
}

Result<void, Error> save_to_file(const String &path, int value) {
    if (path.empty()) {
        return Result<void, Error>::err(Error::invalid_argument("Empty path"));
    }
    if (path == "readonly.txt") {
        return Result<void, Error>::err(Error::permission_denied("File is read-only"));
    }
    std::cout << "  [Saved " << value << " to " << path.c_str() << "]" << std::endl;
    return Result<void, Error>::ok();
}

int main() {
    std::cout << "=== Result Monadic Methods Examples ===" << std::endl;

    // ========================================================================
    // Example 1: if_ok() and if_err() - Simple callbacks
    // ========================================================================
    std::cout << "\n--- Example 1: if_ok() and if_err() ---" << std::endl;

    divide(10, 2)
        .if_ok([](int value) { std::cout << "Success! Result: " << value << std::endl; })
        .if_err([](const Error &err) { std::cout << "Error: " << err.message.c_str() << std::endl; });

    divide(10, 0)
        .if_ok([](int value) { std::cout << "Success! Result: " << value << std::endl; })
        .if_err([](const Error &err) { std::cout << "Error: " << err.message.c_str() << std::endl; });

    // ========================================================================
    // Example 2: match() - Pattern matching style
    // ========================================================================
    std::cout << "\n--- Example 2: match() ---" << std::endl;

    auto result1 = divide(20, 4);
    auto message1 = result1.match(
        [](int value) -> String {
            String msg;
            msg = "Success: ";
            msg += std::to_string(value).c_str();
            return msg;
        },
        [](const Error &err) -> String {
            String msg;
            msg = "Error: ";
            msg += err.message;
            return msg;
        });
    std::cout << message1.c_str() << std::endl;

    auto result2 = divide(20, 0);
    auto message2 = result2.match(
        [](int value) -> String {
            String msg;
            msg = "Success: ";
            msg += std::to_string(value).c_str();
            return msg;
        },
        [](const Error &err) -> String {
            String msg;
            msg = "Error: ";
            msg += err.message;
            return msg;
        });
    std::cout << message2.c_str() << std::endl;

    // ========================================================================
    // Example 3: tap() - Execute callback on both Ok and Err
    // ========================================================================
    std::cout << "\n--- Example 3: tap() ---" << std::endl;

    divide(15, 3)
        .tap([](const auto &val) { std::cout << "  [Logging: Processing value/error]" << std::endl; })
        .if_ok([](int value) { std::cout << "Got value: " << value << std::endl; });

    divide(15, 0)
        .tap([](const auto &val) { std::cout << "  [Logging: Processing value/error]" << std::endl; })
        .if_err([](const Error &err) { std::cout << "Got error: " << err.message.c_str() << std::endl; });

    // ========================================================================
    // Example 4: filter() - Convert Ok to Err if predicate fails
    // ========================================================================
    std::cout << "\n--- Example 4: filter() ---" << std::endl;

    auto filtered1 =
        divide(100, 10)
            .filter([](int x) { return x > 5; }, Error::out_of_range("Value must be > 5"))
            .if_ok([](int x) { std::cout << "Passed filter: " << x << std::endl; })
            .if_err([](const Error &e) { std::cout << "Failed filter: " << e.message.c_str() << std::endl; });

    auto filtered2 =
        divide(10, 5)
            .filter([](int x) { return x > 5; }, Error::out_of_range("Value must be > 5"))
            .if_ok([](int x) { std::cout << "Passed filter: " << x << std::endl; })
            .if_err([](const Error &e) { std::cout << "Failed filter: " << e.message.c_str() << std::endl; });

    // ========================================================================
    // Example 5: zip() - Combine two Results
    // ========================================================================
    std::cout << "\n--- Example 5: zip() ---" << std::endl;

    auto r1 = divide(20, 4);
    auto r2 = divide(30, 5);
    auto zipped = r1.zip(r2).if_ok([](const std::tuple<int, int> &t) {
        std::cout << "Zipped values: (" << std::get<0>(t) << ", " << std::get<1>(t) << ")" << std::endl;
    });

    auto r3 = divide(20, 4);
    auto r4 = divide(30, 0); // This will fail
    auto zipped_err =
        r3.zip(r4)
            .if_ok([](const std::tuple<int, int> &t) {
                std::cout << "Zipped values: (" << std::get<0>(t) << ", " << std::get<1>(t) << ")" << std::endl;
            })
            .if_err([](const Error &e) { std::cout << "Zip failed: " << e.message.c_str() << std::endl; });

    // ========================================================================
    // Example 6: and_() - Return other if this is Ok
    // ========================================================================
    std::cout << "\n--- Example 6: and_() ---" << std::endl;

    auto and_result1 =
        divide(10, 2).and_(parse_int("42")).if_ok([](int x) { std::cout << "and_() success: " << x << std::endl; });

    auto and_result2 =
        divide(10, 0)
            .and_(parse_int("42"))
            .if_ok([](int x) { std::cout << "and_() success: " << x << std::endl; })
            .if_err([](const Error &e) { std::cout << "and_() failed: " << e.message.c_str() << std::endl; });

    // ========================================================================
    // Example 7: or_() - Return this if Ok, otherwise return other
    // ========================================================================
    std::cout << "\n--- Example 7: or_() ---" << std::endl;

    auto or_result1 = divide(10, 2).or_(Result<int, Error>::ok(999)).if_ok([](int x) {
        std::cout << "or_() result (first succeeded): " << x << std::endl;
    });

    auto or_result2 = divide(10, 0).or_(Result<int, Error>::ok(999)).if_ok([](int x) {
        std::cout << "or_() result (fallback used): " << x << std::endl;
    });

    // ========================================================================
    // Example 8: Chaining multiple monadic operations
    // ========================================================================
    std::cout << "\n--- Example 8: Chaining Operations ---" << std::endl;

    parse_int("42")
        .map([](int x) {
            std::cout << "  [Step 1: Parsed value: " << x << "]" << std::endl;
            return x * 2;
        })
        .filter([](int x) { return x < 100; }, Error::out_of_range("Value too large"))
        .and_then([](int x) -> Result<int, Error> {
            std::cout << "  [Step 2: Doubled value: " << x << "]" << std::endl;
            return divide(x, 2);
        })
        .if_ok([](int x) { std::cout << "Final result: " << x << std::endl; })
        .if_err([](const Error &e) { std::cout << "Chain failed: " << e.message.c_str() << std::endl; });

    // ========================================================================
    // Example 9: Result<void, E> with if_ok() and if_err()
    // ========================================================================
    std::cout << "\n--- Example 9: Result<void, E> ---" << std::endl;

    save_to_file("data.txt", 42)
        .if_ok([]() { std::cout << "File saved successfully!" << std::endl; })
        .if_err([](const Error &e) { std::cout << "Save failed: " << e.message.c_str() << std::endl; });

    save_to_file("readonly.txt", 42)
        .if_ok([]() { std::cout << "File saved successfully!" << std::endl; })
        .if_err([](const Error &e) { std::cout << "Save failed: " << e.message.c_str() << std::endl; });

    // ========================================================================
    // Example 10: Result<void, E> with match()
    // ========================================================================
    std::cout << "\n--- Example 10: Result<void, E> with match() ---" << std::endl;

    auto void_result = save_to_file("output.txt", 100);
    auto status = void_result.match([]() -> String { return "SUCCESS"; },
                                    [](const Error &e) -> String {
                                        String msg = "FAILED: ";
                                        msg += e.message;
                                        return msg;
                                    });
    std::cout << "Status: " << status.c_str() << std::endl;

    // ========================================================================
    // Example 11: Complex real-world scenario
    // ========================================================================
    std::cout << "\n--- Example 11: Real-world Pipeline ---" << std::endl;

    auto pipeline = [](const String &input, const String &output_path) {
        return parse_int(input)
            .if_ok([](int x) { std::cout << "  [Parsed input: " << x << "]" << std::endl; })
            .map([](int x) { return x * 10; })
            .if_ok([](int x) { std::cout << "  [Multiplied by 10: " << x << "]" << std::endl; })
            .filter([](int x) { return x >= 100; }, Error::out_of_range("Result must be >= 100"))
            .and_then([&output_path](int x) -> Result<void, Error> {
                std::cout << "  [Validated: " << x << "]" << std::endl;
                return save_to_file(output_path, x);
            })
            .match([]() -> String { return "Pipeline completed successfully!"; },
                   [](const Error &e) -> String {
                       String msg = "Pipeline failed: ";
                       msg += e.message;
                       return msg;
                   });
    };

    std::cout << "Pipeline 1: " << pipeline("42", "result.txt").c_str() << std::endl;
    std::cout << "\nPipeline 2: " << pipeline("NaN", "result.txt").c_str() << std::endl;
    std::cout << "\nPipeline 3: " << pipeline("42", "readonly.txt").c_str() << std::endl;

    return 0;
}
