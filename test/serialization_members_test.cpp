#include "datapod/datapod.hpp"
#include <doctest/doctest.h>

using namespace datapod;

TEST_CASE("Serialization - struct with members()") {
    struct MyData {
        int id = 123;
        double value = 45.67;

        auto members() noexcept { return std::tie(id, value); }

        // Methods
        bool is_valid() const { return id > 0; }
    };

    MyData original;
    original.id = 999;
    original.value = 3.14;

    auto buf = serialize(original);
    auto loaded = deserialize<Mode::NONE, MyData>(buf);

    CHECK(loaded.id == 999);
    CHECK(loaded.value == doctest::Approx(3.14));
}

TEST_CASE("Serialization - exclude runtime members") {
    struct CachedData {
        int data = 42;

        // Don't serialize cache
        mutable bool cached = false;
        mutable int cache_value = 999;

        auto members() noexcept { return std::tie(data); }
    };

    CachedData original;
    original.data = 100;
    original.cached = true;
    original.cache_value = 777;

    auto buf = serialize(original);
    auto loaded = deserialize<Mode::NONE, CachedData>(buf);

    CHECK(loaded.data == 100);
    // Cache not serialized - has default values
    CHECK(loaded.cached == false);
    CHECK(loaded.cache_value == 999);
}

TEST_CASE("Serialization - nested structs with members()") {
    struct Inner {
        int x = 10;
        auto members() noexcept { return std::tie(x); }
    };

    struct Outer {
        Inner inner;
        int y = 20;

        auto members() noexcept { return std::tie(inner, y); }
    };

    Outer original;
    original.inner.x = 111;
    original.y = 222;

    auto buf = serialize(original);
    auto loaded = deserialize<Mode::NONE, Outer>(buf);

    CHECK(loaded.inner.x == 111);
    CHECK(loaded.y == 222);
}

TEST_CASE("Serialization - Vector of structs with members()") {
    struct Item {
        int id = 0;
        double value = 0.0; // Use double instead of String

        auto members() noexcept { return std::tie(id, value); }
    };

    Vector<Item> original;

    Item item1;
    item1.id = 1;
    item1.value = 11.1;
    original.push_back(item1);

    Item item2;
    item2.id = 2;
    item2.value = 22.2;
    original.push_back(item2);

    auto buf = serialize(original);
    auto loaded = deserialize<Mode::NONE, Vector<Item>>(buf);

    CHECK(loaded.size() == 2);
    CHECK(loaded[0].id == 1);
    CHECK(loaded[0].value == doctest::Approx(11.1));
    CHECK(loaded[1].id == 2);
    CHECK(loaded[1].value == doctest::Approx(22.2));
}

TEST_CASE("Serialization - private members exposed") {
    struct SecretData {
      private:
        int secret_id_ = 0;
        double secret_value_ = 0.0;

      public:
        auto members() noexcept { return std::tie(secret_id_, secret_value_); }

        void set_data(int id, double val) {
            secret_id_ = id;
            secret_value_ = val;
        }

        int get_id() const { return secret_id_; }
        double get_value() const { return secret_value_; }
    };

    SecretData original;
    original.set_data(42, 123.45);

    auto buf = serialize(original);
    auto loaded = deserialize<Mode::NONE, SecretData>(buf);

    CHECK(loaded.get_id() == 42);
    CHECK(loaded.get_value() == doctest::Approx(123.45));
}

TEST_CASE("Serialization - partial member selection") {
    struct PartialData {
        int id = 1;
        double value = 0.0;
        int version = 100;

        // Only serialize id and value, not version
        auto members() noexcept { return std::tie(id, value); }
    };

    PartialData original;
    original.id = 777;
    original.value = 88.99;
    original.version = 999; // This won't be serialized

    auto buf = serialize(original);

    PartialData loaded;
    loaded.version = 555; // Set different default
    loaded = deserialize<Mode::NONE, PartialData>(buf);
    loaded.version = 555; // Restore after deserialize

    CHECK(loaded.id == 777);
    CHECK(loaded.value == doctest::Approx(88.99));
    CHECK(loaded.version == 555); // Unchanged, not serialized
}

TEST_CASE("Serialization - fallback to automatic") {
    struct SimpleData {
        int x = 10;
        int y = 20;
    };

    SimpleData original;
    original.x = 111;
    original.y = 222;

    auto buf = serialize(original);
    auto loaded = deserialize<Mode::NONE, SimpleData>(buf);

    CHECK(loaded.x == 111);
    CHECK(loaded.y == 222);
}
