#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "datapod/adapters/lazy.hpp"
#include <string>
#include <thread>
#include <vector>

using namespace datapod;

TEST_SUITE("Lazy") {
    TEST_CASE("Lazy - basic usage") {
        int call_count = 0;
        Lazy<int> lazy([&call_count]() {
            call_count++;
            return 42;
        });

        CHECK(call_count == 0);
        CHECK(!lazy.is_initialized());

        int value = *lazy;
        CHECK(value == 42);
        CHECK(call_count == 1);
        CHECK(lazy.is_initialized());
    }

    TEST_CASE("Lazy - memoization") {
        int call_count = 0;
        Lazy<int> lazy([&call_count]() {
            call_count++;
            return 42;
        });

        // First access
        CHECK(*lazy == 42);
        CHECK(call_count == 1);

        // Second access - should not call function again
        CHECK(*lazy == 42);
        CHECK(call_count == 1);

        // Third access
        CHECK(*lazy == 42);
        CHECK(call_count == 1);
    }

    TEST_CASE("Lazy - get method") {
        Lazy<int> lazy([]() { return 42; });

        CHECK(lazy.get() == 42);
        CHECK(lazy.is_initialized());
    }

    TEST_CASE("Lazy - get_mut") {
        Lazy<int> lazy([]() { return 42; });

        int &value = lazy.get_mut();
        CHECK(value == 42);

        value = 100;
        CHECK(lazy.get() == 100);
    }

    TEST_CASE("Lazy - arrow operator") {
        struct Data {
            int x;
            int y;
        };

        Lazy<Data> lazy([]() { return Data{10, 20}; });

        CHECK(lazy->x == 10);
        CHECK(lazy->y == 20);
        CHECK(lazy.is_initialized());
    }

    TEST_CASE("Lazy - force computation") {
        int call_count = 0;
        Lazy<int> lazy([&call_count]() {
            call_count++;
            return 42;
        });

        CHECK(call_count == 0);
        lazy.force();
        CHECK(call_count == 1);
        CHECK(lazy.is_initialized());
    }

    TEST_CASE("Lazy - peek without forcing") {
        int call_count = 0;
        Lazy<int> lazy([&call_count]() {
            call_count++;
            return 42;
        });

        CHECK(lazy.peek() == nullptr);
        CHECK(call_count == 0);

        lazy.force();
        CHECK(lazy.peek() != nullptr);
        CHECK(*lazy.peek() == 42);
    }

    TEST_CASE("Lazy - take") {
        Lazy<int> lazy([]() { return 42; });

        lazy.force();
        auto value = lazy.take();

        CHECK(value.has_value());
        CHECK(value.value() == 42);
        CHECK(!lazy.is_initialized());
    }

    TEST_CASE("Lazy - take before initialization") {
        Lazy<int> lazy([]() { return 42; });

        auto value = lazy.take();
        CHECK(!value.has_value());
        CHECK(!lazy.is_initialized());
    }

    TEST_CASE("Lazy - reset") {
        int call_count = 0;
        Lazy<int> lazy([&call_count]() {
            call_count++;
            return 42;
        });

        CHECK(*lazy == 42);
        CHECK(call_count == 1);

        lazy.reset();
        CHECK(!lazy.is_initialized());

        // Should recompute
        CHECK(*lazy == 42);
        CHECK(call_count == 2);
    }

    TEST_CASE("Lazy - with string") {
        Lazy<std::string> lazy([]() { return std::string("Hello, World!"); });

        CHECK(*lazy == "Hello, World!");
        CHECK(lazy.is_initialized());
    }

    TEST_CASE("Lazy - with complex computation") {
        Lazy<int> lazy([]() {
            int sum = 0;
            for (int i = 1; i <= 100; ++i) {
                sum += i;
            }
            return sum;
        });

        CHECK(*lazy == 5050);
    }

    TEST_CASE("Lazy - make_lazy helper") {
        auto lazy = make_lazy([]() { return 42; });

        CHECK(*lazy == 42);
    }

    TEST_CASE("Lazy - make_lazy with type deduction") {
        auto lazy = make_lazy([]() { return std::string("test"); });

        CHECK(*lazy == "test");
    }

    TEST_CASE("Lazy - move construction") {
        int call_count = 0;
        Lazy<int> lazy1([&call_count]() {
            call_count++;
            return 42;
        });

        lazy1.force();
        CHECK(call_count == 1);

        Lazy<int> lazy2(std::move(lazy1));
        CHECK(lazy2.is_initialized());
        CHECK(!lazy1.is_initialized());
        CHECK(*lazy2 == 42);
        CHECK(call_count == 1); // No recomputation
    }

    TEST_CASE("Lazy - move assignment") {
        Lazy<int> lazy1([]() { return 42; });
        Lazy<int> lazy2([]() { return 100; });

        lazy1.force();
        lazy2 = std::move(lazy1);

        CHECK(lazy2.is_initialized());
        CHECK(*lazy2 == 42);
    }

    TEST_CASE("Lazy - thread safety") {
        std::atomic<int> call_count{0};
        Lazy<int> lazy([&call_count]() {
            call_count++;
            return 42;
        });

        std::vector<std::thread> threads;
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&lazy]() { lazy.get(); });
        }

        for (auto &t : threads) {
            t.join();
        }

        // Function should be called exactly once despite multiple threads
        CHECK(call_count == 1);
        CHECK(*lazy == 42);
    }

    TEST_CASE("Lazy - expensive computation deferred") {
        bool computed = false;
        Lazy<int> lazy([&computed]() {
            computed = true;
            return 42;
        });

        CHECK(!computed);
        // Lazy value not accessed, computation not performed
    }

    TEST_CASE("Lazy - with non-copyable type") {
        struct NonCopyable {
            int value;
            NonCopyable(int v) : value(v) {}
            NonCopyable(NonCopyable const &) = delete;
            NonCopyable &operator=(NonCopyable const &) = delete;
            NonCopyable(NonCopyable &&) = default;
            NonCopyable &operator=(NonCopyable &&) = default;
        };

        Lazy<NonCopyable> lazy([]() { return NonCopyable{42}; });

        CHECK(lazy->value == 42);
    }

    TEST_CASE("Lazy - multiple accesses via different methods") {
        int call_count = 0;
        Lazy<int> lazy([&call_count]() {
            call_count++;
            return 42;
        });

        CHECK(lazy.get() == 42);
        CHECK(call_count == 1);

        CHECK(*lazy == 42);
        CHECK(call_count == 1);

        CHECK(lazy.get_mut() == 42);
        CHECK(call_count == 1);
    }

    TEST_CASE("Lazy - reset and recompute") {
        int value = 1;
        Lazy<int> lazy([&value]() { return value * 10; });

        CHECK(*lazy == 10);

        value = 2;
        lazy.reset();

        CHECK(*lazy == 20); // Recomputed with new value
    }

    TEST_CASE("Lazy - peek after take") {
        Lazy<int> lazy([]() { return 42; });

        lazy.force();
        lazy.take();

        CHECK(lazy.peek() == nullptr);
        CHECK(!lazy.is_initialized());
    }
}
