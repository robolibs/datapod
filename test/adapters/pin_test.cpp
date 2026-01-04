#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <datapod/pods/adapters/pin.hpp>
#include <string>

using namespace datapod;

// Test type that is Unpin (default)
struct UnpinType {
    int value;
};

// Test type that is NOT Unpin (self-referential)
struct NotUnpinType {
    int *ptr;
    int value;

    NotUnpinType() : value(42) { ptr = &value; }

    // Verify self-reference is valid
    bool is_valid() const { return ptr == &value && *ptr == value; }
};

// Mark NotUnpinType as !Unpin
template <> struct datapod::Unpin<NotUnpinType> : std::false_type {};

TEST_CASE("Pin - Basic construction with Unpin type") {
    int value = 42;
    auto pinned = Pin<int *>::new_pin(&value);

    CHECK(*pinned == 42);
    CHECK(pinned.get() == &value);
}

TEST_CASE("Pin - Unchecked construction") {
    int value = 42;
    auto pinned = Pin<int *>::new_unchecked(&value);

    CHECK(*pinned == 42);
    CHECK(pinned.get() == &value);
}

TEST_CASE("Pin - Helper function pin()") {
    int value = 42;
    auto pinned = pin(&value);

    CHECK(*pinned == 42);
}

TEST_CASE("Pin - Helper function pin_unchecked()") {
    int value = 42;
    auto pinned = pin_unchecked(&value);

    CHECK(*pinned == 42);
}

TEST_CASE("Pin - Const access") {
    int value = 42;
    auto pinned = pin(&value);

    const int *ptr = pinned.get();
    CHECK(*ptr == 42);
}

TEST_CASE("Pin - Mutable access for Unpin types") {
    int value = 42;
    auto pinned = pin(&value);

    int *ptr = pinned.get_mut();
    *ptr = 100;

    CHECK(value == 100);
    CHECK(*pinned == 100);
}

TEST_CASE("Pin - Dereference operator") {
    int value = 42;
    auto pinned = pin(&value);

    CHECK(*pinned == 42);

    *pinned = 100;
    CHECK(value == 100);
}

TEST_CASE("Pin - Arrow operator with struct") {
    UnpinType obj{42};
    auto pinned = pin(&obj);

    CHECK(pinned->value == 42);

    pinned->value = 100;
    CHECK(obj.value == 100);
}

TEST_CASE("Pin - Bool conversion") {
    int value = 42;
    auto pinned = pin(&value);

    CHECK(static_cast<bool>(pinned));

    auto null_pin = Pin<int *>::new_unchecked(nullptr);
    CHECK_FALSE(static_cast<bool>(null_pin));
}

TEST_CASE("Pin - Equality comparison") {
    int value1 = 42;
    int value2 = 100;

    auto pin1 = pin(&value1);
    auto pin2 = pin(&value1);
    auto pin3 = pin(&value2);

    CHECK(pin1 == pin2);
    CHECK(pin1 != pin3);
}

TEST_CASE("Pin - Less-than comparison") {
    int arr[2] = {1, 2};

    auto pin1 = pin(&arr[0]);
    auto pin2 = pin(&arr[1]);

    CHECK(pin1 < pin2);
}

TEST_CASE("Pin - into_inner for Unpin types") {
    int value = 42;
    auto pinned = pin(&value);

    int *ptr = pinned.into_inner();
    CHECK(ptr == &value);
    CHECK(*ptr == 42);
}

TEST_CASE("Pin - get_unchecked_mut") {
    int value = 42;
    auto pinned = pin(&value);

    int *ptr = pinned.get_unchecked_mut();
    *ptr = 100;

    CHECK(value == 100);
}

TEST_CASE("Pin - Reference specialization") {
    int value = 42;
    auto pinned = Pin<int &>::new_pin(value);

    CHECK(*pinned == 42);
}

TEST_CASE("Pin - Reference get()") {
    int value = 42;
    auto pinned = Pin<int &>::new_pin(value);

    const int &ref = pinned.get();
    CHECK(ref == 42);
}

TEST_CASE("Pin - Reference get_mut()") {
    int value = 42;
    auto pinned = Pin<int &>::new_pin(value);

    int &ref = pinned.get_mut();
    ref = 100;

    CHECK(value == 100);
}

TEST_CASE("Pin - Reference dereference") {
    int value = 42;
    auto pinned = Pin<int &>::new_pin(value);

    CHECK(*pinned == 42);

    *pinned = 100;
    CHECK(value == 100);
}

TEST_CASE("Pin - Reference arrow operator") {
    UnpinType obj{42};
    auto pinned = Pin<UnpinType &>::new_pin(obj);

    CHECK(pinned->value == 42);

    pinned->value = 100;
    CHECK(obj.value == 100);
}

TEST_CASE("Pin - Reference into_inner") {
    int value = 42;
    auto pinned = Pin<int &>::new_pin(value);

    int &ref = pinned.into_inner();
    ref = 100;

    CHECK(value == 100);
}

TEST_CASE("Pin - Reference get_unchecked_mut") {
    int value = 42;
    auto pinned = Pin<int &>::new_pin(value);

    int &ref = pinned.get_unchecked_mut();
    ref = 100;

    CHECK(value == 100);
}

TEST_CASE("Pin - NotUnpin type const access") {
    NotUnpinType obj;
    auto pinned = Pin<NotUnpinType *>::new_unchecked(&obj);

    CHECK(pinned->is_valid());
    CHECK(pinned.get()->is_valid());
}

TEST_CASE("Pin - NotUnpin type unchecked mut access") {
    NotUnpinType obj;
    auto pinned = Pin<NotUnpinType *>::new_unchecked(&obj);

    NotUnpinType *ptr = pinned.get_unchecked_mut();
    CHECK(ptr->is_valid());
}

TEST_CASE("Pin - Const pointer") {
    const int value = 42;
    auto pinned = Pin<const int *>::new_unchecked(&value);

    CHECK(*pinned == 42);
    CHECK(pinned.get() == &value);
}

TEST_CASE("Pin - String type") {
    std::string str = "hello";
    auto pinned = pin(&str);

    CHECK(*pinned == "hello");
    CHECK(pinned->size() == 5);

    *pinned = "world";
    CHECK(str == "world");
}

TEST_CASE("Pin - Unpin trait check") {
    CHECK(Unpin<int>::value);
    CHECK(Unpin<UnpinType>::value);
    CHECK_FALSE(Unpin<NotUnpinType>::value);
}

TEST_CASE("Pin - Multiple pins to same object") {
    int value = 42;

    auto pin1 = pin(&value);
    auto pin2 = pin(&value);

    CHECK(pin1 == pin2);
    CHECK(*pin1 == *pin2);

    *pin1 = 100;
    CHECK(*pin2 == 100);
}

TEST_CASE("Pin - Nested struct") {
    struct Outer {
        struct Inner {
            int value;
        } inner;
    };

    Outer obj{{42}};
    auto pinned = pin(&obj);

    CHECK(pinned->inner.value == 42);

    pinned->inner.value = 100;
    CHECK(obj.inner.value == 100);
}

TEST_CASE("Pin - Array element") {
    int arr[3] = {1, 2, 3};

    auto pin0 = pin(&arr[0]);
    auto pin1 = pin(&arr[1]);
    auto pin2 = pin(&arr[2]);

    CHECK(*pin0 == 1);
    CHECK(*pin1 == 2);
    CHECK(*pin2 == 3);

    *pin1 = 20;
    CHECK(arr[1] == 20);
}

TEST_CASE("Pin - Reference new_unchecked") {
    int value = 42;
    auto pinned = Pin<int &>::new_unchecked(value);

    CHECK(*pinned == 42);
}

TEST_CASE("Pin - Pointer to pointer") {
    int value = 42;
    int *ptr = &value;
    auto pinned = pin(&ptr);

    CHECK(**pinned == 42);

    **pinned = 100;
    CHECK(value == 100);
}
