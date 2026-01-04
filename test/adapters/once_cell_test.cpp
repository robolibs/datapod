#include <doctest/doctest.h>

#include "datapod/pods/adapters/once_cell.hpp"
#include <string>
#include <thread>
#include <vector>

using namespace datapod;

TEST_SUITE("OnceCell") {
    TEST_CASE("OnceCell - default construction") {
        OnceCell<int> cell;
        CHECK(cell.is_initialized() == false);
        CHECK(cell.get() == nullptr);
    }

    TEST_CASE("OnceCell - set and get") {
        OnceCell<int> cell;

        auto result = cell.set(42);
        CHECK(result.is_ok() == true);
        CHECK(cell.is_initialized() == true);

        auto value = cell.get();
        CHECK(value != nullptr);
        CHECK(*value == 42);
    }

    TEST_CASE("OnceCell - set twice fails") {
        OnceCell<int> cell;

        auto result1 = cell.set(42);
        CHECK(result1.is_ok() == true);

        auto result2 = cell.set(100);
        CHECK(result2.is_err() == true);
        CHECK(result2.error() == 100); // Returns the value that couldn't be set

        // Original value unchanged
        CHECK(*cell.get() == 42);
    }

    TEST_CASE("OnceCell - get_mut") {
        OnceCell<int> cell;
        cell.set(42);

        auto mut_ptr = cell.get_mut();
        CHECK(mut_ptr != nullptr);

        *mut_ptr = 100;
        CHECK(*cell.get() == 100);
    }

    TEST_CASE("OnceCell - get_or_init") {
        OnceCell<int> cell;

        int &value = cell.get_or_init([] { return 42; });
        CHECK(value == 42);
        CHECK(cell.is_initialized() == true);

        // Second call returns same value without calling function
        int call_count = 0;
        int &value2 = cell.get_or_init([&call_count] {
            call_count++;
            return 100;
        });
        CHECK(value2 == 42);    // Still original value
        CHECK(call_count == 0); // Function not called
    }

    TEST_CASE("OnceCell - get_or_try_init success") {
        OnceCell<int> cell;

        auto result = cell.get_or_try_init([] { return Optional<int>(42); });
        CHECK(result != nullptr);
        CHECK(*result == 42);
        CHECK(cell.is_initialized() == true);
    }

    TEST_CASE("OnceCell - get_or_try_init failure") {
        OnceCell<int> cell;

        auto result = cell.get_or_try_init([] { return Optional<int>(nullopt); });
        CHECK(result == nullptr);
        CHECK(cell.is_initialized() == false);
    }

    TEST_CASE("OnceCell - take") {
        OnceCell<int> cell;
        cell.set(42);

        auto value = cell.take();
        CHECK(value.has_value() == true);
        CHECK(value.value() == 42);
        CHECK(cell.is_initialized() == false);

        // Can set again after take
        auto result = cell.set(100);
        CHECK(result.is_ok() == true);
        CHECK(*cell.get() == 100);
    }

    TEST_CASE("OnceCell - unwrap") {
        OnceCell<int> cell;
        cell.set(42);

        CHECK(cell.unwrap() == 42);
    }

    TEST_CASE("OnceCell - unwrap on uninitialized throws") {
        OnceCell<int> cell;
        CHECK_THROWS_AS(cell.unwrap(), std::runtime_error);
    }

    TEST_CASE("OnceCell - unwrap_mut") {
        OnceCell<int> cell;
        cell.set(42);

        cell.unwrap_mut() = 100;
        CHECK(cell.unwrap() == 100);
    }

    TEST_CASE("OnceCell - unwrap_mut on uninitialized throws") {
        OnceCell<int> cell;
        CHECK_THROWS_AS(cell.unwrap_mut(), std::runtime_error);
    }

    TEST_CASE("OnceCell - get_or_default") {
        OnceCell<int> cell;

        int value = cell.get_or_default();
        CHECK(value == 0); // Default for int

        cell.set(42);
        value = cell.get_or_default();
        CHECK(value == 42);
    }

    TEST_CASE("OnceCell - into_inner") {
        OnceCell<int> cell;
        cell.set(42);

        auto value = cell.into_inner();
        CHECK(value.has_value() == true);
        CHECK(value.value() == 42);
        CHECK(cell.is_initialized() == false);
    }

    TEST_CASE("OnceCell - move construction") {
        OnceCell<int> cell1;
        cell1.set(42);

        OnceCell<int> cell2(std::move(cell1));
        CHECK(cell2.is_initialized() == true);
        CHECK(*cell2.get() == 42);
        CHECK(cell1.is_initialized() == false);
    }

    TEST_CASE("OnceCell - move assignment") {
        OnceCell<int> cell1;
        cell1.set(42);

        OnceCell<int> cell2;
        cell2 = std::move(cell1);

        CHECK(cell2.is_initialized() == true);
        CHECK(*cell2.get() == 42);
        CHECK(cell1.is_initialized() == false);
    }

    TEST_CASE("OnceCell - with string") {
        OnceCell<std::string> cell;

        cell.set("Hello, World!");
        CHECK(*cell.get() == "Hello, World!");

        auto taken = cell.take();
        CHECK(*taken == "Hello, World!");
    }

    TEST_CASE("OnceCell - thread safety") {
        OnceCell<int> cell;
        std::vector<std::thread> threads;
        std::atomic<int> success_count{0};

        // Try to set from multiple threads
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&cell, &success_count, i]() {
                auto result = cell.set(i);
                if (result.is_ok()) {
                    success_count++;
                }
            });
        }

        for (auto &t : threads) {
            t.join();
        }

        // Only one thread should succeed
        CHECK(success_count == 1);
        CHECK(cell.is_initialized() == true);

        // Value should be one of the attempted values (0-9)
        int value = *cell.get();
        CHECK(value >= 0);
        CHECK(value < 10);
    }

    TEST_CASE("OnceCell - get_or_init thread safety") {
        OnceCell<int> cell;
        std::vector<std::thread> threads;
        std::atomic<int> init_count{0};

        // Try to initialize from multiple threads
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&cell, &init_count]() {
                cell.get_or_init([&init_count]() {
                    init_count++;
                    return 42;
                });
            });
        }

        for (auto &t : threads) {
            t.join();
        }

        // Initialization function should be called exactly once
        CHECK(init_count == 1);
        CHECK(*cell.get() == 42);
    }

    TEST_CASE("OnceCell - complex type") {
        struct Data {
            int x;
            std::string s;

            Data(int x_, std::string s_) : x(x_), s(std::move(s_)) {}
        };

        OnceCell<Data> cell;
        cell.set(Data{42, "test"});

        auto &data = cell.unwrap();
        CHECK(data.x == 42);
        CHECK(data.s == "test");
    }
}
