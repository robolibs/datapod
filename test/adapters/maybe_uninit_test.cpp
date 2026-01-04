#include <doctest/doctest.h>

#include "datapod/pods/adapters/maybe_uninit.hpp"
#include <string>

using namespace datapod;

TEST_SUITE("MaybeUninit") {
    TEST_CASE("MaybeUninit - uninit construction") {
        MaybeUninit<int> mu;
        // Memory is uninitialized - we can't safely read it
        CHECK(mu.as_ptr() != nullptr);
    }

    TEST_CASE("MaybeUninit - init construction") {
        MaybeUninit<int> mu(42);
        CHECK(mu.assume_init_ref() == 42);
        mu.drop();
    }

    TEST_CASE("MaybeUninit - uninit static method") {
        auto mu = MaybeUninit<int>::uninit();
        CHECK(mu.as_ptr() != nullptr);
    }

    TEST_CASE("MaybeUninit - init static method") {
        auto mu = MaybeUninit<int>::init(42);
        CHECK(mu.assume_init_ref() == 42);
        mu.drop();
    }

    TEST_CASE("MaybeUninit - write value") {
        auto mu = MaybeUninit<int>::uninit();
        mu.write(42);
        CHECK(mu.assume_init_ref() == 42);
        mu.drop();
    }

    TEST_CASE("MaybeUninit - write rvalue") {
        auto mu = MaybeUninit<std::string>::uninit();
        mu.write(std::string("hello"));
        CHECK(mu.assume_init_ref() == "hello");
        mu.drop();
    }

    TEST_CASE("MaybeUninit - as_ptr") {
        auto mu = MaybeUninit<int>::init(42);
        int *ptr = mu.as_ptr();
        CHECK(ptr != nullptr);
        CHECK(*ptr == 42);
        mu.drop();
    }

    TEST_CASE("MaybeUninit - assume_init_mut") {
        auto mu = MaybeUninit<int>::init(42);
        int &ref = mu.assume_init_mut();
        CHECK(ref == 42);

        ref = 100;
        CHECK(mu.assume_init_ref() == 100);
        mu.drop();
    }

    TEST_CASE("MaybeUninit - assume_init_ref") {
        auto mu = MaybeUninit<int>::init(42);
        int const &ref = mu.assume_init_ref();
        CHECK(ref == 42);
        mu.drop();
    }

    TEST_CASE("MaybeUninit - assume_init") {
        auto mu = MaybeUninit<int>::init(42);
        int value = mu.assume_init();
        CHECK(value == 42);
        // Note: mu is now in moved-from state, don't call drop
    }

    TEST_CASE("MaybeUninit - drop") {
        static int destructor_count = 0;

        struct Tracked {
            ~Tracked() { destructor_count++; }
        };

        destructor_count = 0;
        {
            auto mu = MaybeUninit<Tracked>::init(Tracked{});
            CHECK(destructor_count == 1); // Temporary destroyed
            mu.drop();
            CHECK(destructor_count == 2); // mu destroyed
        }
    }

    TEST_CASE("MaybeUninit - zeroed") {
        auto mu = MaybeUninit<int>::uninit();
        mu.zeroed();
        // Memory is now zeroed
        CHECK(*mu.as_ptr() == 0);
    }

    TEST_CASE("MaybeUninit - with string") {
        auto mu = MaybeUninit<std::string>::uninit();
        mu.write("Hello, World!");
        CHECK(mu.assume_init_ref() == "Hello, World!");
        mu.drop();
    }

    TEST_CASE("MaybeUninit - manual initialization pattern") {
        auto mu = MaybeUninit<int>::uninit();

        // Manually initialize
        new (mu.as_mut_ptr()) int(42);

        CHECK(mu.assume_init_ref() == 42);
        mu.drop();
    }

    TEST_CASE("MaybeUninit - deferred initialization") {
        auto mu = MaybeUninit<std::string>::uninit();

        // Do some work...
        bool should_init = true;

        if (should_init) {
            mu.write("initialized");
            CHECK(mu.assume_init_ref() == "initialized");
            mu.drop();
        }
    }

    TEST_CASE("MaybeUninit - array of uninit") {
        MaybeUninit<int> array[5];

        // Initialize each element
        for (int i = 0; i < 5; ++i) {
            array[i].write(i * 10);
        }

        // Check values
        for (int i = 0; i < 5; ++i) {
            CHECK(array[i].assume_init_ref() == i * 10);
        }

        // Drop all
        for (int i = 0; i < 5; ++i) {
            array[i].drop();
        }
    }

    TEST_CASE("MaybeUninit - performance optimization example") {
        // Avoid default construction + assignment
        auto mu = MaybeUninit<std::string>::uninit();

        // Directly construct in place
        mu.write("Direct construction - no default ctor");

        CHECK(mu.assume_init_ref() == "Direct construction - no default ctor");
        mu.drop();
    }

    TEST_CASE("MaybeUninit - complex type") {
        struct Data {
            int x;
            std::string s;
            Data(int x_, std::string s_) : x(x_), s(std::move(s_)) {}
        };

        auto mu = MaybeUninit<Data>::uninit();
        mu.write(Data{42, "test"});

        CHECK(mu.assume_init_ref().x == 42);
        CHECK(mu.assume_init_ref().s == "test");
        mu.drop();
    }

    TEST_CASE("MaybeUninit - uninit helper") {
        auto mu = uninit<int>();
        mu.write(42);
        CHECK(mu.assume_init_ref() == 42);
        mu.drop();
    }

    TEST_CASE("MaybeUninit - init helper") {
        auto mu = init(42);
        CHECK(mu.assume_init_ref() == 42);
        mu.drop();
    }

    TEST_CASE("MaybeUninit - trivially destructible type") {
        auto mu = MaybeUninit<int>::init(42);
        // For trivially destructible types, drop is a no-op
        mu.drop();
        // Safe to call multiple times for trivial types
        mu.drop();
    }

    TEST_CASE("MaybeUninit - non-trivially destructible type") {
        static int destructor_count = 0;

        struct NonTrivial {
            int *ptr;
            NonTrivial() : ptr(new int(42)) {}
            ~NonTrivial() {
                delete ptr;
                destructor_count++;
            }
        };

        destructor_count = 0;
        {
            auto mu = MaybeUninit<NonTrivial>::init(NonTrivial{});
            CHECK(destructor_count == 1); // Temporary
            mu.drop();
            CHECK(destructor_count == 2); // Explicit drop
        }
    }

    TEST_CASE("MaybeUninit - partial initialization") {
        struct Pair {
            int first;
            int second;
        };

        auto mu = MaybeUninit<Pair>::uninit();

        // Manually initialize fields
        Pair *ptr = mu.as_ptr();
        ptr->first = 10;
        ptr->second = 20;

        CHECK(mu.assume_init_ref().first == 10);
        CHECK(mu.assume_init_ref().second == 20);
        mu.drop();
    }

    TEST_CASE("MaybeUninit - move semantics") {
        auto mu = MaybeUninit<std::string>::init("movable");

        std::string value = mu.assume_init();
        CHECK(value == "movable");
        // mu is now in moved-from state
    }
}
