#include <cstdint>
#include <datapod/datapod.hpp>
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== LIFO (Stack) ===\n";
    Lifo<int> s;
    s.push(1);
    s.push(2);
    s.push(3);
    while (!s.empty()) {
        std::cout << s.top() << " ";
        s.pop();
    }
    std::cout << "\n\n";

    std::cout << "=== FIFO (Queue) ===\n";
    Fifo<std::uint32_t> q;
    q.push(10);
    q.push(20);
    q.push(30);
    while (!q.empty()) {
        std::cout << q.front() << " ";
        q.pop();
    }
    std::cout << "\n\n";

    std::cout << "=== Fixed FIFO (no overwrite) ===\n";
    FixedFifo<int, 3> fixed;
    fixed.push(1);
    fixed.push(2);
    fixed.push(3);
    std::cout << "front=" << fixed.front() << " back=" << fixed.back() << "\n";
    fixed.pop();
    fixed.push(4);
    while (!fixed.empty()) {
        std::cout << fixed.front() << " ";
        fixed.pop();
    }
    std::cout << "\n\n";

    std::cout << "=== Fixed FIFO (overwrite oldest) ===\n";
    OverwritingFifo<int, 3> ow;
    ow.push(1);
    ow.push(2);
    ow.push(3);
    ow.push(4); // overwrites 1
    std::cout << "front=" << ow.front() << " back=" << ow.back() << "\n";
    while (!ow.empty()) {
        std::cout << ow.front() << " ";
        ow.pop();
    }
    std::cout << "\n";
}

