#include "datapod/reflection/has_members.hpp"
#include <doctest/doctest.h>
#include <tuple>

using namespace datapod;

TEST_CASE("has_members detection - basic") {
    struct WithMembers {
        int x;
        auto members() { return std::tie(x); }
    };

    struct WithoutMembers {
        int x, y;
    };

    CHECK(has_members_v<WithMembers> == true);
    CHECK(has_members_v<WithoutMembers> == false);
}

TEST_CASE("has_members detection - const overload") {
    struct ConstMembers {
        int x;
        auto members() const { return std::tie(x); }
    };

    CHECK(has_const_members_v<ConstMembers> == true);
}

TEST_CASE("has_members detection - both overloads") {
    struct BothMembers {
        int x;
        auto members() { return std::tie(x); }
        auto members() const { return std::tie(x); }
    };

    CHECK(has_members_v<BothMembers> == true);
    CHECK(has_const_members_v<BothMembers> == true);
}

TEST_CASE("has_members detection - wrong signature") {
    struct WrongSignature {
        int x;
        void members() {} // Returns void, not tuple
    };

    // Should still detect it exists, but returns_tuple will fail
    CHECK(has_members_v<WrongSignature> == true);
    CHECK(detail::members_returns_tuple_v<WrongSignature> == false);
}

TEST_CASE("has_members detection - returns tuple-like") {
    struct ReturnsTuple {
        int x, y;
        auto members() { return std::tie(x, y); }
    };

    struct ReturnsInt {
        int x;
        int members() { return x; }
    };

    CHECK(detail::members_returns_tuple_v<ReturnsTuple> == true);
    CHECK(detail::members_returns_tuple_v<ReturnsInt> == false);
}

TEST_CASE("has_members detection - private members exposed") {
    struct PrivateData {
      private:
        int x_ = 42;

      public:
        auto members() { return std::tie(x_); }
    };

    CHECK(has_members_v<PrivateData> == true);
}

TEST_CASE("has_members detection - multiple members") {
    struct MultipleMembers {
        int a, b, c;
        auto members() { return std::tie(a, b, c); }
    };

    CHECK(has_members_v<MultipleMembers> == true);
    CHECK(detail::members_returns_tuple_v<MultipleMembers> == true);
}

TEST_CASE("has_members detection - noexcept") {
    struct NoexceptMembers {
        int x;
        auto members() noexcept { return std::tie(x); }
    };

    CHECK(has_members_v<NoexceptMembers> == true);
}

TEST_CASE("has_members detection - only const version") {
    struct OnlyConst {
        int x;
        auto members() const { return std::tie(x); }
    };

    // Const members() can be called on non-const object too
    CHECK(has_members_v<OnlyConst> == true); // Const version works for both
    CHECK(has_const_members_v<OnlyConst> == true);
}

// Helper template for testing
template <typename T> struct TemplatedStruct {
    T value;
    auto members() { return std::tie(value); }
};

TEST_CASE("has_members detection - templated struct") {
    CHECK(has_members_v<TemplatedStruct<int>> == true);
    CHECK(has_members_v<TemplatedStruct<double>> == true);
}
