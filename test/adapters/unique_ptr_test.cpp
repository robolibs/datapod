#include <datapod/pods/adapters/unique_ptr.hpp>
#include <doctest/doctest.h>
#include <string>
#include <utility>

using namespace datapod;

// Helper struct to track object lifecycle
struct LifecycleTracker {
    static int constructions;
    static int destructions;
    int value;

    LifecycleTracker(int v = 0) : value(v) { ++constructions; }
    ~LifecycleTracker() { ++destructions; }

    static void reset_counts() {
        constructions = 0;
        destructions = 0;
    }
};

int LifecycleTracker::constructions = 0;
int LifecycleTracker::destructions = 0;

// Polymorphic test classes
struct Base {
    int base_value = 10;
    virtual ~Base() = default;
    virtual int get_value() const { return base_value; }
};

struct Derived : Base {
    int derived_value = 20;
    int get_value() const override { return derived_value; }
};

// Struct with members for pointer access testing
struct TestPoint {
    int x;
    int y;
    TestPoint(int x_, int y_) : x(x_), y(y_) {}
};

TEST_SUITE("UniquePtr") {

    // ============================================================================
    // Construction Tests
    // ============================================================================

    TEST_CASE("DefaultConstruction") {
        UniquePtr<int> ptr;
        CHECK(ptr.get() == nullptr);
        CHECK_FALSE(ptr);
    }

    TEST_CASE("NullptrConstruction") {
        UniquePtr<int> ptr(nullptr);
        CHECK(ptr.get() == nullptr);
        CHECK_FALSE(ptr);
    }

    TEST_CASE("PointerConstruction") {
        UniquePtr<int> ptr(new int(42));
        CHECK(ptr.get() != nullptr);
        CHECK(ptr);
        CHECK(*ptr == 42);
    }

    TEST_CASE("MoveConstruction") {
        UniquePtr<int> ptr1(new int(100));
        int *raw = ptr1.get();

        UniquePtr<int> ptr2(std::move(ptr1));

        CHECK(ptr1.get() == nullptr);
        CHECK(ptr2.get() == raw);
        CHECK(*ptr2 == 100);
    }

    TEST_CASE("MoveConstructionFromNullptr") {
        UniquePtr<int> ptr1;
        UniquePtr<int> ptr2(std::move(ptr1));

        CHECK(ptr1.get() == nullptr);
        CHECK(ptr2.get() == nullptr);
    }

    // ============================================================================
    // Assignment Tests
    // ============================================================================

    TEST_CASE("MoveAssignment") {
        UniquePtr<int> ptr1(new int(200));
        UniquePtr<int> ptr2(new int(300));
        int *raw1 = ptr1.get();

        ptr2 = std::move(ptr1);

        CHECK(ptr1.get() == nullptr);
        CHECK(ptr2.get() == raw1);
        CHECK(*ptr2 == 200);
    }

    TEST_CASE("MoveAssignmentToEmpty") {
        UniquePtr<int> ptr1(new int(50));
        UniquePtr<int> ptr2;
        int *raw1 = ptr1.get();

        ptr2 = std::move(ptr1);

        CHECK(ptr1.get() == nullptr);
        CHECK(ptr2.get() == raw1);
        CHECK(*ptr2 == 50);
    }

    TEST_CASE("MoveAssignmentFromEmpty") {
        UniquePtr<int> ptr1;
        UniquePtr<int> ptr2(new int(75));

        ptr2 = std::move(ptr1);

        CHECK(ptr1.get() == nullptr);
        CHECK(ptr2.get() == nullptr);
    }

    TEST_CASE("SelfMoveAssignment") {
        UniquePtr<int> ptr(new int(999));
        int *raw = ptr.get();

        // NOLINTBEGIN(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
        // Intentionally testing self-move assignment behavior
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
        ptr = std::move(ptr);
#pragma GCC diagnostic pop
        // NOLINTEND(bugprone-use-after-move,clang-analyzer-cplusplus.Move)

        CHECK(ptr.get() == raw);
        CHECK(*ptr == 999);
    }

    TEST_CASE("NullptrAssignment") {
        UniquePtr<int> ptr(new int(123));
        ptr = nullptr;

        CHECK(ptr.get() == nullptr);
        CHECK_FALSE(ptr);
    }

    // ============================================================================
    // Observer Tests
    // ============================================================================

    TEST_CASE("Get") {
        UniquePtr<int> ptr(new int(42));
        int *raw = ptr.get();

        CHECK(raw != nullptr);
        CHECK(*raw == 42);
    }

    TEST_CASE("GetFromNullptr") {
        UniquePtr<int> ptr;
        CHECK(ptr.get() == nullptr);
    }

    TEST_CASE("Dereference") {
        UniquePtr<int> ptr(new int(777));
        CHECK(*ptr == 777);

        *ptr = 888;
        CHECK(*ptr == 888);
    }

    TEST_CASE("MemberAccess") {
        UniquePtr<TestPoint> ptr(new TestPoint(10, 20));

        CHECK(ptr->x == 10);
        CHECK(ptr->y == 20);

        ptr->x = 30;
        CHECK(ptr->x == 30);
    }

    TEST_CASE("BoolConversionTrue") {
        UniquePtr<int> ptr(new int(1));
        CHECK(ptr);
        CHECK(static_cast<bool>(ptr) == true);
    }

    TEST_CASE("BoolConversionFalse") {
        UniquePtr<int> ptr;
        CHECK_FALSE(ptr);
        CHECK(static_cast<bool>(ptr) == false);
    }

    // ============================================================================
    // Modifier Tests
    // ============================================================================

    TEST_CASE("Release") {
        UniquePtr<int> ptr(new int(42));
        int *raw = ptr.release();

        CHECK(ptr.get() == nullptr);
        CHECK(*raw == 42);

        delete raw; // Manual cleanup required
    }

    TEST_CASE("ReleaseFromNullptr") {
        UniquePtr<int> ptr;
        int *raw = ptr.release();

        CHECK(raw == nullptr);
        CHECK(ptr.get() == nullptr);
    }

    TEST_CASE("ResetWithPointer") {
        LifecycleTracker::reset_counts();
        UniquePtr<LifecycleTracker> ptr(new LifecycleTracker(10));
        CHECK(LifecycleTracker::constructions == 1);
        CHECK(LifecycleTracker::destructions == 0);

        ptr.reset(new LifecycleTracker(20));
        CHECK(LifecycleTracker::constructions == 2);
        CHECK(LifecycleTracker::destructions == 1);
        CHECK(ptr->value == 20);
    }

    TEST_CASE("ResetToNullptr") {
        LifecycleTracker::reset_counts();
        UniquePtr<LifecycleTracker> ptr(new LifecycleTracker(5));
        CHECK(LifecycleTracker::destructions == 0);

        ptr.reset();
        CHECK(LifecycleTracker::destructions == 1);
        CHECK(ptr.get() == nullptr);
    }

    TEST_CASE("ResetAlreadyNull") {
        UniquePtr<int> ptr;
        ptr.reset(); // Should be safe
        CHECK(ptr.get() == nullptr);
    }

    TEST_CASE("Swap") {
        UniquePtr<int> ptr1(new int(111));
        UniquePtr<int> ptr2(new int(222));
        int *raw1 = ptr1.get();
        int *raw2 = ptr2.get();

        ptr1.swap(ptr2);

        CHECK(ptr1.get() == raw2);
        CHECK(ptr2.get() == raw1);
        CHECK(*ptr1 == 222);
        CHECK(*ptr2 == 111);
    }

    TEST_CASE("SwapWithNullptr") {
        UniquePtr<int> ptr1(new int(500));
        UniquePtr<int> ptr2;
        int *raw1 = ptr1.get();

        ptr1.swap(ptr2);

        CHECK(ptr1.get() == nullptr);
        CHECK(ptr2.get() == raw1);
        CHECK(*ptr2 == 500);
    }

    TEST_CASE("SwapBothNull") {
        UniquePtr<int> ptr1;
        UniquePtr<int> ptr2;

        ptr1.swap(ptr2);

        CHECK(ptr1.get() == nullptr);
        CHECK(ptr2.get() == nullptr);
    }

    // ============================================================================
    // make_unique Tests
    // ============================================================================

    TEST_CASE("MakeUniqueBasic") {
        auto ptr = make_unique<int>(42);
        CHECK(ptr.get() != nullptr);
        CHECK(*ptr == 42);
    }

    TEST_CASE("MakeUniqueNoArgs") {
        auto ptr = make_unique<int>();
        CHECK(ptr.get() != nullptr);
        // Value is default-initialized (unspecified for int)
    }

    TEST_CASE("MakeUniqueWithArgs") {
        auto ptr = make_unique<TestPoint>(15, 25);
        CHECK(ptr->x == 15);
        CHECK(ptr->y == 25);
    }

    TEST_CASE("MakeUniqueComplex") {
        auto ptr = make_unique<std::string>("Hello UniquePtr");
        CHECK(*ptr == "Hello UniquePtr");
    }

    // ============================================================================
    // Comparison Tests
    // ============================================================================

    TEST_CASE("EqualityBothNull") {
        UniquePtr<int> ptr1;
        UniquePtr<int> ptr2;
        CHECK(ptr1 == ptr2);
    }

    TEST_CASE("EqualitySamePointer") {
        int *raw = new int(10);
        UniquePtr<int> ptr1(raw);
        // Can't test directly since can't have two UniquePtrs to same object
        CHECK(ptr1 == ptr1);
    }

    TEST_CASE("InequalityDifferent") {
        UniquePtr<int> ptr1(new int(1));
        UniquePtr<int> ptr2(new int(2));
        CHECK(ptr1 != ptr2);
    }

    TEST_CASE("InequalityNullVsNonNull") {
        UniquePtr<int> ptr1(new int(5));
        UniquePtr<int> ptr2;
        CHECK(ptr1 != ptr2);
        CHECK(ptr2 != ptr1);
    }

    TEST_CASE("CompareWithNullptrEqual") {
        UniquePtr<int> ptr;
        CHECK(ptr == nullptr);
        CHECK(nullptr == ptr);
    }

    TEST_CASE("CompareWithNullptrNotEqual") {
        UniquePtr<int> ptr(new int(10));
        CHECK(ptr != nullptr);
        CHECK(nullptr != ptr);
    }

    // ============================================================================
    // RAII / Lifetime Tests
    // ============================================================================

    TEST_CASE("AutomaticCleanup") {
        LifecycleTracker::reset_counts();

        {
            UniquePtr<LifecycleTracker> ptr(new LifecycleTracker(100));
            CHECK(LifecycleTracker::constructions == 1);
            CHECK(LifecycleTracker::destructions == 0);
        }

        // Object should be deleted when ptr goes out of scope
        CHECK(LifecycleTracker::destructions == 1);
    }

    TEST_CASE("MovePreservesLifetime") {
        LifecycleTracker::reset_counts();

        {
            UniquePtr<LifecycleTracker> ptr1(new LifecycleTracker(50));
            {
                UniquePtr<LifecycleTracker> ptr2(std::move(ptr1));
                CHECK(LifecycleTracker::destructions == 0);
            }
            // ptr2 destroyed, object deleted
            CHECK(LifecycleTracker::destructions == 1);
        }
        // ptr1 destroyed but it's null, no additional deletion
        CHECK(LifecycleTracker::destructions == 1);
    }

    TEST_CASE("AssignmentDeletesOld") {
        LifecycleTracker::reset_counts();

        UniquePtr<LifecycleTracker> ptr1(new LifecycleTracker(1));
        UniquePtr<LifecycleTracker> ptr2(new LifecycleTracker(2));
        CHECK(LifecycleTracker::constructions == 2);
        CHECK(LifecycleTracker::destructions == 0);

        ptr1 = std::move(ptr2);
        // Old ptr1 object should be deleted
        CHECK(LifecycleTracker::destructions == 1);
    }

    // ============================================================================
    // Polymorphism Tests
    // ============================================================================

    TEST_CASE("BasePointer") {
        UniquePtr<Base> ptr(new Derived);
        CHECK(ptr->base_value == 10);
        CHECK(ptr->get_value() == 20); // Virtual call to Derived
    }

    TEST_CASE("VirtualDestructor") {
        // This test verifies that virtual destructors work correctly
        // If Base didn't have virtual destructor, this would be undefined behavior
        UniquePtr<Base> ptr(new Derived);
        ptr.reset(); // Should call Derived destructor through Base*
        CHECK(ptr.get() == nullptr);
    }

    // ============================================================================
    // Edge Cases
    // ============================================================================

    TEST_CASE("MoveFromMovedFrom") {
        UniquePtr<int> ptr1(new int(10));
        UniquePtr<int> ptr2(std::move(ptr1));
        UniquePtr<int> ptr3(std::move(ptr1)); // Move from already-moved-from object

        CHECK(ptr1.get() == nullptr);
        CHECK(ptr3.get() == nullptr);
        CHECK(*ptr2 == 10);
    }

    TEST_CASE("ReleaseAfterRelease") {
        UniquePtr<int> ptr(new int(20));
        int *raw1 = ptr.release();
        int *raw2 = ptr.release();

        CHECK(raw2 == nullptr);
        delete raw1;
    }

    TEST_CASE("ResetAfterRelease") {
        UniquePtr<int> ptr(new int(30));
        int *raw = ptr.release();

        ptr.reset(new int(40));
        CHECK(*ptr == 40);

        delete raw;
    }

    TEST_CASE("MultipleSwaps") {
        UniquePtr<int> ptr1(new int(1));
        UniquePtr<int> ptr2(new int(2));
        int *raw1 = ptr1.get();
        int *raw2 = ptr2.get();

        ptr1.swap(ptr2);
        ptr1.swap(ptr2);

        CHECK(ptr1.get() == raw1);
        CHECK(ptr2.get() == raw2);
    }

    // ============================================================================
    // Complex Type Tests
    // ============================================================================

    TEST_CASE("ManageString") {
        auto ptr = make_unique<std::string>("DataPod UniquePtr");
        CHECK(ptr->length() == 17);
        ptr->append(" Test");
        CHECK(*ptr == "DataPod UniquePtr Test");
    }

    TEST_CASE("ResourceTracking") {
        LifecycleTracker::reset_counts();

        auto ptr1 = make_unique<LifecycleTracker>(10);
        auto ptr2 = make_unique<LifecycleTracker>(20);
        auto ptr3 = make_unique<LifecycleTracker>(30);

        CHECK(LifecycleTracker::constructions == 3);
        CHECK(LifecycleTracker::destructions == 0);

        ptr1.reset();
        CHECK(LifecycleTracker::destructions == 1);

        ptr2 = std::move(ptr3);
        CHECK(LifecycleTracker::destructions == 2);
    }

    TEST_CASE("MoveSemantics") {
        // Verify that move operations don't leak
        LifecycleTracker::reset_counts();

        {
            auto ptr = make_unique<LifecycleTracker>(100);
            auto ptr2 = std::move(ptr);
            auto ptr3 = std::move(ptr2);
            auto ptr4 = std::move(ptr3);

            CHECK(LifecycleTracker::constructions == 1);
            CHECK(LifecycleTracker::destructions == 0);
            CHECK(ptr4->value == 100);
        }

        CHECK(LifecycleTracker::destructions == 1);
    }
}
