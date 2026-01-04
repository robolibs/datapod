#include <datapod/pods/adapters/unique_ptr.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace datapod;

// ============================================================================
// Helper Classes
// ============================================================================

class Resource {
  public:
    std::string name;
    int id;

    Resource(std::string n, int i) : name(std::move(n)), id(i) {
        std::cout << "  Resource '" << name << "' (id=" << id << ") created\n";
    }

    ~Resource() { std::cout << "  Resource '" << name << "' (id=" << id << ") destroyed\n"; }

    void use() const { std::cout << "  Using resource: " << name << " (id=" << id << ")\n"; }
};

class FileHandle {
  public:
    std::string filename;

    explicit FileHandle(std::string fname) : filename(std::move(fname)) {
        std::cout << "  Opening file: " << filename << "\n";
    }

    ~FileHandle() { std::cout << "  Closing file: " << filename << "\n"; }

    void write(const std::string &data) { std::cout << "  Writing to " << filename << ": " << data << "\n"; }
};

struct Point {
    int x, y;
    Point(int x_, int y_) : x(x_), y(y_) {}
};

class Base {
  public:
    virtual ~Base() { std::cout << "  Base destroyed\n"; }
    virtual void print() const { std::cout << "  Base object\n"; }
};

class Derived : public Base {
  public:
    ~Derived() override { std::cout << "  Derived destroyed\n"; }
    void print() const override { std::cout << "  Derived object\n"; }
};

// ============================================================================
// Example Functions
// ============================================================================

void example_basic_usage() {
    std::cout << "\n=== Basic Usage ===\n";

    // Create UniquePtr with raw pointer
    UniquePtr<int> ptr1(new int(42));
    std::cout << "Value: " << *ptr1 << "\n";
    std::cout << "Address: " << ptr1.get() << "\n";

    // Check if non-null
    if (ptr1) {
        std::cout << "ptr1 is valid\n";
    }

    // Modify value
    *ptr1 = 100;
    std::cout << "Modified value: " << *ptr1 << "\n";

    // Automatic cleanup when ptr1 goes out of scope
    std::cout << "ptr1 will be automatically deleted\n";
}

void example_make_unique() {
    std::cout << "\n=== Make Unique (Preferred Creation) ===\n";

    // Preferred way to create UniquePtr - safer and more concise
    auto ptr1 = make_unique<int>(42);
    std::cout << "Integer value: " << *ptr1 << "\n";

    // With multiple arguments
    auto ptr2 = make_unique<Point>(10, 20);
    std::cout << "Point: (" << ptr2->x << ", " << ptr2->y << ")\n";

    // With complex types
    auto ptr3 = make_unique<std::string>("Hello UniquePtr");
    std::cout << "String: " << *ptr3 << "\n";
}

void example_move_semantics() {
    std::cout << "\n=== Move Semantics ===\n";

    auto ptr1 = make_unique<Resource>("Resource A", 1);
    std::cout << "Created ptr1\n";

    // Move ownership from ptr1 to ptr2
    UniquePtr<Resource> ptr2(std::move(ptr1));
    std::cout << "Moved to ptr2\n";

    // ptr1 is now null
    if (!ptr1) {
        std::cout << "ptr1 is now null\n";
    }

    // ptr2 owns the resource
    if (ptr2) {
        std::cout << "ptr2 owns the resource:\n";
        ptr2->use();
    }
}

void example_move_assignment() {
    std::cout << "\n=== Move Assignment ===\n";

    auto ptr1 = make_unique<Resource>("Resource 1", 10);
    auto ptr2 = make_unique<Resource>("Resource 2", 20);

    std::cout << "Before assignment:\n";
    ptr1->use();
    ptr2->use();

    // Move assignment - Resource 1 will be destroyed, ptr1 takes Resource 2
    std::cout << "\nExecuting move assignment:\n";
    ptr1 = std::move(ptr2);

    std::cout << "\nAfter assignment:\n";
    ptr1->use();

    if (!ptr2) {
        std::cout << "ptr2 is now null\n";
    }
}

void example_release() {
    std::cout << "\n=== Release (Transfer Ownership) ===\n";

    auto ptr = make_unique<Resource>("Transferable Resource", 100);

    std::cout << "UniquePtr owns the resource\n";
    ptr->use();

    // Transfer ownership - UniquePtr gives up ownership, returns raw pointer
    std::cout << "\nReleasing ownership:\n";
    Resource *raw = ptr.release();

    if (!ptr) {
        std::cout << "UniquePtr is now null\n";
    }

    std::cout << "Raw pointer owns the resource:\n";
    raw->use();

    // Manual cleanup required!
    std::cout << "\nManual cleanup:\n";
    delete raw;
}

void example_reset() {
    std::cout << "\n=== Reset (Replace Managed Object) ===\n";

    auto ptr = make_unique<Resource>("Original", 1);
    ptr->use();

    // Reset to new resource - old one is deleted
    std::cout << "\nResetting to new resource:\n";
    ptr.reset(new Resource("Replacement", 2));
    ptr->use();

    // Reset to nullptr - deletes resource
    std::cout << "\nResetting to nullptr:\n";
    ptr.reset();

    if (!ptr) {
        std::cout << "UniquePtr is now null\n";
    }
}

void example_swap() {
    std::cout << "\n=== Swap (Exchange Ownership) ===\n";

    auto ptr1 = make_unique<Resource>("First", 1);
    auto ptr2 = make_unique<Resource>("Second", 2);

    std::cout << "Before swap:\n";
    ptr1->use();
    ptr2->use();

    std::cout << "\nSwapping:\n";
    ptr1.swap(ptr2);

    std::cout << "\nAfter swap:\n";
    ptr1->use();
    ptr2->use();
}

void example_nullptr_handling() {
    std::cout << "\n=== Nullptr Handling ===\n";

    // Default construction
    UniquePtr<int> ptr1;
    std::cout << "Default constructed: " << (ptr1 ? "valid" : "null") << "\n";

    // Explicit nullptr construction
    UniquePtr<int> ptr2(nullptr);
    std::cout << "Nullptr constructed: " << (ptr2 ? "valid" : "null") << "\n";

    // Comparison with nullptr
    if (ptr1 == nullptr) {
        std::cout << "ptr1 == nullptr: true\n";
    }

    if (nullptr == ptr2) {
        std::cout << "nullptr == ptr2: true\n";
    }

    // Assign nullptr
    auto ptr3 = make_unique<int>(42);
    std::cout << "Before nullptr assignment: " << (ptr3 ? "valid" : "null") << "\n";
    ptr3 = nullptr;
    std::cout << "After nullptr assignment: " << (ptr3 ? "valid" : "null") << "\n";
}

void example_comparison() {
    std::cout << "\n=== Comparison Operations ===\n";

    auto ptr1 = make_unique<int>(10);
    auto ptr2 = make_unique<int>(20);
    UniquePtr<int> ptr3;
    UniquePtr<int> ptr4;

    // Compare different pointers
    std::cout << "ptr1 != ptr2: " << (ptr1 != ptr2 ? "true" : "false") << "\n";

    // Compare null pointers
    std::cout << "ptr3 == ptr4 (both null): " << (ptr3 == ptr4 ? "true" : "false") << "\n";

    // Compare with nullptr
    std::cout << "ptr1 != nullptr: " << (ptr1 != nullptr ? "true" : "false") << "\n";
    std::cout << "ptr3 == nullptr: " << (ptr3 == nullptr ? "true" : "false") << "\n";
}

void example_raii_pattern() {
    std::cout << "\n=== RAII Pattern (Automatic Resource Management) ===\n";

    std::cout << "Entering scope:\n";
    {
        auto file = make_unique<FileHandle>("data.txt");
        file->write("Important data");
        file->write("More data");

        std::cout << "\nLeaving scope - file will auto-close:\n";
    }
    std::cout << "Scope exited, file closed automatically\n";
}

UniquePtr<Resource> create_resource(const std::string &name, int id) { return make_unique<Resource>(name, id); }

void example_factory_function() {
    std::cout << "\n=== Factory Function (Return UniquePtr) ===\n";

    std::cout << "Creating resource via factory:\n";
    auto resource = create_resource("Factory Product", 999);
    resource->use();

    std::cout << "\nResource will be destroyed when leaving scope\n";
}

void example_polymorphism() {
    std::cout << "\n=== Polymorphism ===\n";

    // Store derived object in base pointer
    UniquePtr<Base> ptr1(new Derived);

    std::cout << "Calling virtual function:\n";
    ptr1->print(); // Calls Derived::print()

    // Reset triggers virtual destructor
    std::cout << "\nResetting (triggers virtual destructor):\n";
    ptr1.reset();

    std::cout << "\nCreating another derived object:\n";
    auto ptr2 = make_unique<Derived>();
    ptr2->print();

    std::cout << "\nLeaving scope (virtual destructor called):\n";
}

void example_use_case_file_handle() {
    std::cout << "\n=== Use Case: File Handle Management ===\n";

    auto process_file = [](const std::string &filename) {
        auto file = make_unique<FileHandle>(filename);

        file->write("Processing data...");
        file->write("More processing...");

        // If exception occurs here, file is still closed automatically
        file->write("Done!");

        // File automatically closed when function returns
    };

    std::cout << "Processing file 1:\n";
    process_file("config.cfg");

    std::cout << "\nProcessing file 2:\n";
    process_file("data.dat");
}

void example_use_case_dynamic_objects() {
    std::cout << "\n=== Use Case: Managing Dynamic Objects ===\n";

    struct Database {
        std::string name;
        Database(std::string n) : name(std::move(n)) { std::cout << "  Database '" << name << "' connected\n"; }
        ~Database() { std::cout << "  Database '" << name << "' disconnected\n"; }
        void query(const std::string &sql) { std::cout << "  Executing: " << sql << "\n"; }
    };

    auto db = make_unique<Database>("UserDB");
    db->query("SELECT * FROM users");
    db->query("INSERT INTO logs VALUES ('login')");

    std::cout << "Database will auto-disconnect\n";
}

void example_container_of_unique_ptr() {
    std::cout << "\n=== Container of UniquePtr ===\n";

    std::vector<UniquePtr<Resource>> resources;

    std::cout << "Adding resources to vector:\n";
    resources.push_back(make_unique<Resource>("Resource A", 1));
    resources.push_back(make_unique<Resource>("Resource B", 2));
    resources.push_back(make_unique<Resource>("Resource C", 3));

    std::cout << "\nUsing resources:\n";
    for (const auto &res : resources) {
        res->use();
    }

    std::cout << "\nClearing vector (destroys all resources):\n";
    resources.clear();

    std::cout << "All resources destroyed\n";
}

void example_move_only_in_class() {
    std::cout << "\n=== Move-Only Class Member ===\n";

    class Service {
      public:
        UniquePtr<Resource> resource;

        explicit Service(std::string name, int id) : resource(make_unique<Resource>(std::move(name), id)) {
            std::cout << "  Service created\n";
        }

        // Move constructor
        Service(Service &&other) noexcept : resource(std::move(other.resource)) { std::cout << "  Service moved\n"; }

        // Move assignment
        Service &operator=(Service &&other) noexcept {
            resource = std::move(other.resource);
            std::cout << "  Service move-assigned\n";
            return *this;
        }

        // Copy disabled (because UniquePtr is move-only)
        Service(const Service &) = delete;
        Service &operator=(const Service &) = delete;

        ~Service() { std::cout << "  Service destroyed\n"; }

        void execute() {
            if (resource) {
                resource->use();
            }
        }
    };

    Service srv1("Service Resource", 777);
    srv1.execute();

    std::cout << "\nMoving service:\n";
    Service srv2(std::move(srv1));
    srv2.execute();

    if (!srv1.resource) {
        std::cout << "srv1 no longer has resource\n";
    }

    std::cout << "\nServices will be destroyed:\n";
}

void example_exception_safety() {
    std::cout << "\n=== Exception Safety ===\n";

    auto simulate_exception = []() {
        auto res = make_unique<Resource>("Exception Test", 404);
        res->use();

        std::cout << "\nSimulating error (not actually throwing):\n";
        std::cout << "// If exception occurred here, resource would still be cleaned up\n";

        // Resource automatically cleaned up even if exception thrown
    };

    simulate_exception();
    std::cout << "Resource was properly cleaned up\n";
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "================================================\n";
    std::cout << "DataPod UniquePtr Usage Examples\n";
    std::cout << "================================================\n";

    example_basic_usage();
    example_make_unique();
    example_move_semantics();
    example_move_assignment();
    example_release();
    example_reset();
    example_swap();
    example_nullptr_handling();
    example_comparison();
    example_raii_pattern();
    example_factory_function();
    example_polymorphism();
    example_use_case_file_handle();
    example_use_case_dynamic_objects();
    example_container_of_unique_ptr();
    example_move_only_in_class();
    example_exception_safety();

    std::cout << "\n================================================\n";
    std::cout << "All examples completed successfully!\n";
    std::cout << "================================================\n";

    return 0;
}
