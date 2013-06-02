
#include <iostream>
#include "common.hpp"
#include "allocators.hpp"

int main() {
    std::allocator<int> alloc;
    Whisper::WrapAllocator<int, std::allocator> walloc(alloc);
    std::cout << "Whisper says hello." << std::endl;
}
